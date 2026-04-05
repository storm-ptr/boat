// Andrew Naplavkov

#ifndef BOAT_GEOMETRY_FIBONACCI_HPP
#define BOAT_GEOMETRY_FIBONACCI_HPP

#include <boat/geometry/algorithm.hpp>
#include <boat/geometry/detail/inverse_fibonacci.hpp>
#include <generator>
#include <queue>
#include <unordered_set>

namespace boat::geometry {

template <box T>
auto box_fibonacci(T const& mbr, size_t num_points)
{
    auto a = mbr.min_corner(), b = mbr.max_corner();
    return std::views::iota(0u, num_points) |
           std::views::transform([=](auto i) -> d2_of<T>::point {
               return {std::lerp(a.x(), b.x(), frac(i * numbers::inv_phi)),
                       std::lerp(a.y(), b.y(), (i + .5) / num_points)};
           });
}

struct priority_point {
    geographic::point point;
    double priority;
    size_t index;

    priority_point(  //
        geographic::point const& p,
        geographic::point const& max,
        size_t index)
        : point{p}, priority{-comparable_distance(p, max)}, index{index}
    {
    }

    friend bool operator<(priority_point const& lhs, priority_point const& rhs)
    {
        return lhs.priority < rhs.priority;
    }
};

struct geographic_fibonacci {
    size_t num_points;

    geographic::point operator[](size_t i) const
    {
        auto azimuth = 2 * numbers::pi * frac(i * numbers::inv_phi);
        auto polar = std::acos(1 - 2 * (i + .5) / num_points);
        return {azimuth * numbers::radian - 180, polar * numbers::radian - 90};
    }

    size_t nearest(geographic::point const& p) const
    {
        auto indices = inverse_fibonacci(p, num_points);
        auto proj = [&](auto i) { return priority_point{(*this)[i], p, i}; };
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
            for (auto ring = buf(next).outer(); auto item : ring)
                for (auto i : inverse_fibonacci(item, num_points))
                    if (done.insert(i).second)
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

constexpr auto geographic_fibonacci_levels =
    std::views::iota(0uz) |
    std::views::transform([](auto n) { return pow2(2 * n); }) |
    std::views::take_while([](auto n) { return !!n; }) |
    std::views::transform([](auto n) { return geographic_fibonacci(n); });

}  // namespace boat::geometry

#endif  // BOAT_GEOMETRY_FIBONACCI_HPP
