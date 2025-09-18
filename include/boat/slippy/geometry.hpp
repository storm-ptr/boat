// Andrew Naplavkov

#ifndef BOAT_SLIPPY_GEOMETRY_HPP
#define BOAT_SLIPPY_GEOMETRY_HPP

#include <boat/geometry/model.hpp>
#include <boat/slippy/detail/utility.hpp>
#include <boost/accumulators/accumulators.hpp>
#include <boost/accumulators/statistics/mean.hpp>
#include <boost/accumulators/statistics/stats.hpp>
#include <queue>
#include <set>

namespace boat::slippy {

inline geometry::geographic::box envelope(tile const& t)
{
    return {{tilex2lon(t.x, t.z), tiley2lat(t.y + 1, t.z)},
            {tilex2lon(t.x + 1, t.z), tiley2lat(t.y, t.z)}};
}

inline tile to_tile(geometry::geographic::point const& ll, int z)
{
    return {lon2tilex(ll.x(), z), lat2tiley(ll.y(), z), z};
}

inline std::unordered_set<tile> to_tiles(geometry::geographic::grid const& grid,
                                         double resolution)
{
    namespace ba = boost::accumulators;
    static constexpr auto d = {-1, 0, 1};
    auto ret = std::unordered_set<tile>{};
    if (grid.empty())
        return ret;
    auto step = resolution * 256.;
    auto lat = ba::accumulator_set<double, ba::stats<ba::tag::mean>>{};
    for (auto& ll : grid | std::views::values | std::views::join)
        lat(ll.y());
    auto z = zoom(step, ba::mean(lat));
    for (auto& ll : grid | std::views::values | std::views::join) {
        auto processed = std::unordered_set{{to_tile(ll, z)}};
        auto queue = std::queue{std::from_range, processed};
        while (!queue.empty()) {
            auto top = queue.front();
            queue.pop();
            ret.insert(top);
            for (auto [dx, dy] : std::views::cartesian_product(d, d)) {
                auto t = corrected({top.x + dx, top.y + dy, top.z});
                if (!processed.insert(t).second)
                    continue;
                auto c = boost::geometry::return_centroid<
                    geometry::geographic::point>(envelope(t));
                auto lim = std::max<>(step, steps[t.z] * step_factor(c.y()));
                if (boost::geometry::distance(ll, c) > lim)
                    continue;
                queue.push(t);
            }
        }
    }
    return ret;
}

}  // namespace boat::slippy

#endif  // BOAT_SLIPPY_GEOMETRY_HPP
