// Andrew Naplavkov

#ifndef BOAT_GEOMETRY_FIBONACCI_HPP
#define BOAT_GEOMETRY_FIBONACCI_HPP

#include <boat/geometry/algorithm.hpp>
#include <boost/container/static_vector.hpp>
#include <boost/qvm/mat_operations.hpp>
#include <boost/qvm/vec_mat_operations.hpp>
#include <boost/qvm/vec_operations.hpp>
#include <generator>
#include <queue>
#include <unordered_set>

namespace boat::geometry {

struct priority_point {
    geographic::point pos;
    double priority;
    size_t n;

    priority_point(geographic::point a, geographic::point const& b, size_t n)
        : pos{std::move(a)}, priority{-comparable_distance(pos, b)}, n{n}
    {
    }

    friend bool operator<(priority_point const& lhs, priority_point const& rhs)
    {
        return lhs.priority < rhs.priority;
    }
};

template <class T = boost::container::static_vector<size_t, 4>>
T inverse_fibonacci(geographic::point const& p, size_t num_points)
{
    namespace num = numbers;
    static constexpr auto d = {0., 1.};
    if (num_points <= 4)
        return std::views::iota(0u, num_points) | std::ranges::to<T>();
    auto azimuth = (p.x() + 180) * num::degree;
    auto polar = (p.y() + 90) * num::degree;
    auto k = .5 * num::inv_ln_phi *
             std::log(num::sqrt_5 * num::pi * num_points *
                      std::pow(std::sin(polar), 2));
    k = std::max<>(2., std::floor(k));
    auto fk = std::pow(num::phi, static_cast<size_t>(k)) / num::sqrt_5;
    auto f0 = std::round(fk);
    auto f1 = std::round(fk * num::phi);
    auto inv_size = 1. / num_points;
    auto b = boost::qvm::mat{
        {{2 * num::pi * (frac((f0 + 1) * num::inv_phi) - num::inv_phi),
          2 * num::pi * (frac((f1 + 1) * num::inv_phi) - num::inv_phi)},
         {-2 * f0 * inv_size, -2 * f1 * inv_size}}};
    auto c =
        inverse(b) * boost::qvm::vec{{azimuth, std::cos(polar) - 1 + inv_size}};
    X(c) = std::floor(X(c));
    Y(c) = std::floor(Y(c));
    auto ret = T{};
    for (auto [x, y] : std::views::cartesian_product(d, d)) {
        auto z = dot(row<1>(b), c + boost::qvm::vec{{x, y}}) + 1 - inv_size;
        z = 2 * std::clamp(z, -1., 1.) - z;
        ret.push_back(static_cast<size_t>(num_points * (1 - z) / 2));
    }
    return ret;
}

struct fibonacci {
    size_t num_points;

    geographic::point operator[](size_t n) const
    {
        auto azimuth = 2 * numbers::pi * frac(n * numbers::inv_phi);
        auto polar = std::acos(1 - 2 * (n + .5) / num_points);
        return {azimuth * numbers::radian - 180, polar * numbers::radian - 90};
    }

    size_t nearest(geographic::point const& p) const
    {
        auto indices = inverse_fibonacci(p, num_points);
        auto proj = [&](auto n) { return priority_point{(*this)[n], p, n}; };
        return *std::ranges::max_element(indices, std::less{}, proj);
    }

    template <std::predicate<geographic::point const&> S  //
              = decltype([](auto&&) { return false; })>
    std::generator<size_t> nearests(geographic::point p, S sentinel = {}) const
    {
        auto done = std::unordered_set<size_t>{};
        auto queue = std::priority_queue<priority_point>{};
        auto buf = buffer(numbers::earth::sqrt_area / std::sqrt(num_points), 4);
        for (auto next = p;;) {
            for (auto v : buf(next).outer())
                for (auto i : inverse_fibonacci(v, num_points))
                    if (done.insert(i).second)
                        if (auto q = (*this)[i]; !sentinel(q))
                            queue.emplace(std::move(q), p, i);
            if (queue.empty())
                break;
            auto top = queue.top();
            queue.pop();
            co_yield top.n;
            next = top.pos;
        }
    }
};

constexpr auto fibonacci_levels =
    std::views::iota(0uz) |
    std::views::transform([](auto n) { return pow2(2 * n); }) |
    std::views::transform([](auto n) { return fibonacci(n); });

}  // namespace boat::geometry

#endif  // BOAT_GEOMETRY_FIBONACCI_HPP
