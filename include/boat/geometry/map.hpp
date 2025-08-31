// Andrew Naplavkov

#ifndef BOAT_GEOMETRY_MAP_HPP
#define BOAT_GEOMETRY_MAP_HPP

#include <boat/geometry/detail/distribution.hpp>
#include <boost/accumulators/accumulators.hpp>
#include <boost/accumulators/statistics/mean.hpp>
#include <boost/accumulators/statistics/stats.hpp>

namespace boat::geometry {

double scale(projection auto const& pj,
             geographic::point const& ll_center,
             double resolution)
{
    namespace ba = boost::accumulators;
    auto dist = ba::accumulator_set<double, ba::stats<ba::tag::mean>>{};
    if (auto xy_center = cartesian::point{}; pj.forward(ll_center, xy_center))
        for (auto ll : buffer(resolution, 4)(ll_center).outer())
            if (auto xy = cartesian::point{}; pj.forward(ll, xy))
                dist(boost::geometry::distance(xy_center, xy));
    return ba::mean(dist);
}

inline cartesian::box envelope(cartesian::point const& xy_center,
                               double scale,
                               int width,
                               int height)
{
    auto ret =
        cartesian::box{{-width / 2., -height / 2.}, {width / 2., height / 2.}};
    boost::geometry::multiply_value(ret.min_corner(), scale);
    boost::geometry::multiply_value(ret.max_corner(), scale);
    boost::geometry::add_point(ret.min_corner(), xy_center);
    boost::geometry::add_point(ret.max_corner(), xy_center);
    return ret;
}

geographic::grid inverse(projection auto const& pj,
                         cartesian::box const& mbr,
                         size_t num_points)
{
    auto lls = geographic::multi_point{};
    for (auto xy : box_fibonacci(mbr, num_points))
        if (!pj.inverse(xy, lls.emplace_back()))
            lls.pop_back();
    auto accept = [&](geographic::point const& ll) {
        auto xy = cartesian::point{};
        return pj.forward(ll, xy) && boost::geometry::within(xy, mbr);
    };
    auto ret = geographic::grid{};
    for (auto z : std::views::iota(0)) {
        auto fib = geographic_fibonacci{static_cast<size_t>(std::pow(2, z))};
        auto indices = std::unordered_set<size_t>{};
        for (auto& ll : lls)
            for (auto i : fib.nearests(ll, accept)) {
                if (!indices.insert(i).second)
                    break;
                if (indices.size() > lls.size())
                    return ret;
            }
        auto& lvl = ret[root_geoid_area / std::sqrt(fib.num_points)];
        lvl.resize(indices.size());
        for (auto [i, j] : indices | std::views::enumerate)
            lvl[i] = fib[j];
    }
    return ret;
}

}  // namespace boat::geometry

#endif  // BOAT_GEOMETRY_MAP_HPP
