// Andrew Naplavkov

#ifndef BOAT_GEOMETRY_DISTRIBUTION_HPP
#define BOAT_GEOMETRY_DISTRIBUTION_HPP

#include <boat/geometry/algorithm.hpp>
#include <boat/geometry/detail/inverse_fibonacci.hpp>
#include <boat/geometry/detail/priority_point.hpp>
#include <generator>
#include <queue>
#include <unordered_set>

namespace boat::geometry {

template <box B>
auto box_interpolate(B const& mbr, size_t num_points)
{
    using point_t = d2<coord_sys<B>>::point;
    auto ret = boost::geometry::model::multi_point<point_t>{};
    for (auto tuple : boost::geometry::box_view{mbr} | std::views::pairwise) {
        auto a = std::get<0>(tuple), b = std::get<1>(tuple);
        ret.push_back(a);
        ret.append_range(
            std::views::iota(0u, num_points) |
            std::views::transform([=](auto i) -> point_t {
                auto t = (i + 1.) / (num_points + 1.);
                return {std::lerp(a.x(), b.x(), t), std::lerp(a.y(), b.y(), t)};
            }));
    }
    return ret;
}

template <box B>
auto box_fibonacci(B const& mbr, size_t num_points)
{
    auto a = mbr.min_corner(), b = mbr.max_corner();
    return std::views::iota(0u, num_points) |
           std::views::transform([=](auto i) -> d2<coord_sys<B>>::point {
               return {std::lerp(a.x(), b.x(), frac(i * inv_phi)),
                       std::lerp(a.y(), b.y(), (i + .5) / num_points)};
           });
}

struct geographic_fibonacci {
    size_t num_points;

    geographic::point operator[](size_t i) const
    {
        auto azimuthal = 2 * pi * frac(i * inv_phi);
        auto polar = std::acos(1 - 2 * (i + .5) / num_points);
        return {azimuthal * radian - 180, polar * radian - 90};
    }

    size_t nearest(geographic::point const& p) const
    {
        auto neighbors = inverse_fibonacci(p, num_points);
        auto proj = [&](auto i) { return priority_point{(*this)[i], p, i}; };
        return *std::ranges::max_element(neighbors, std::less{}, proj);
    }

    template <std::predicate<geographic::point const&> P =
                  decltype([](geographic::point const&) { return true; })>
    std::generator<size_t> nearests(geographic::point p, P accept = {}) const
    {
        auto queue = std::priority_queue<priority_point>{};
        auto processed = std::unordered_set<size_t>{};
        auto buf = buffer(root_geoid_area / std::sqrt(num_points), 4);
        for (auto next = p;;) {
            for (auto ring = buf(next).outer(); auto item : ring)
                for (auto i : inverse_fibonacci(item, num_points))
                    if (processed.insert(i).second)
                        if (auto q = (*this)[i]; accept(q))
                            queue.emplace(q, p, i);
            if (queue.empty())
                break;
            auto top = queue.top();
            queue.pop();
            co_yield top.value;
            next = top.point;
        }
    }
};

}  // namespace boat::geometry

#endif  // BOAT_GEOMETRY_DISTRIBUTION_HPP
