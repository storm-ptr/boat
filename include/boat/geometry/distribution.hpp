// Andrew Naplavkov

#ifndef BOAT_GEOMETRY_DISTRIBUTION_HPP
#define BOAT_GEOMETRY_DISTRIBUTION_HPP

#include <boat/geometry/algorithm.hpp>
#include <boat/geometry/detail/inverse_fibonacci.hpp>
#include <boat/geometry/detail/priority_point.hpp>
#include <generator>
#include <queue>
#include <random>
#include <unordered_set>

namespace boat::geometry {

inline std::generator<geographic::point> random()
{
    auto gen = std::mt19937{std::random_device()()};
    auto dist = std::uniform_real_distribution<double>{0, 1};
    for (;;) {
        auto azimuthal = 2 * numbers::pi * dist(gen);
        auto polar = std::acos(1 - 2 * dist(gen));
        co_yield {azimuthal * numbers::rad - 180, polar * numbers::rad - 90};
    }
}

template <box T>
std::generator<typename as<T>::point> box_interpolate(T mbr, size_t num_points)
{
    for (auto tuple : boost::geometry::box_view{mbr} | std::views::pairwise) {
        auto a = std::get<0>(tuple), b = std::get<1>(tuple);
        co_yield a;
        co_yield std::ranges::elements_of(
            std::views::iota(0u, num_points) |
            std::views::transform([=](auto i) -> as<T>::point {
                auto t = (i + 1.) / (num_points + 1.);
                return {std::lerp(a.x(), b.x(), t), std::lerp(a.y(), b.y(), t)};
            }));
    }
}

template <box T>
auto box_fibonacci(T const& mbr, size_t num_points)
{
    auto a = mbr.min_corner(), b = mbr.max_corner();
    return std::views::iota(0u, num_points) |
           std::views::transform([=](auto i) -> as<T>::point {
               return {std::lerp(a.x(), b.x(), frac(i * numbers::inv_phi)),
                       std::lerp(a.y(), b.y(), (i + .5) / num_points)};
           });
}

struct geographic_fibonacci {
    size_t num_points;

    geographic::point operator[](size_t i) const
    {
        auto azimuthal = 2 * numbers::pi * frac(i * numbers::inv_phi);
        auto polar = std::acos(1 - 2 * (i + .5) / num_points);
        return {azimuthal * numbers::rad - 180, polar * numbers::rad - 90};
    }

    size_t nearest(geographic::point const& p) const
    {
        auto neighbors = inverse_fibonacci(p, num_points);
        auto proj = [&](auto i) { return priority_point{(*this)[i], p, i}; };
        return *std::ranges::max_element(neighbors, std::less{}, proj);
    }

    template <std::predicate<geographic::point const&> S =
                  decltype([](geographic::point const&) { return false; })>
    std::generator<size_t> nearests(geographic::point p, S sentinel = {}) const
    {
        auto queue = std::priority_queue<priority_point>{};
        auto processed = std::unordered_set<size_t>{};
        auto buf = buffer(numbers::earth::sqrt_area / std::sqrt(num_points), 4);
        for (auto next = p;;) {
            for (auto ring = buf(next).outer(); auto item : ring)
                for (auto i : inverse_fibonacci(item, num_points))
                    if (processed.insert(i).second)
                        if (auto q = (*this)[i]; !sentinel(q))
                            queue.emplace(q, p, i);
            if (queue.empty())
                break;
            auto top = queue.top();
            queue.pop();
            co_yield top.index;
            next = top.point;
        }
    }
};

}  // namespace boat::geometry

#endif  // BOAT_GEOMETRY_DISTRIBUTION_HPP
