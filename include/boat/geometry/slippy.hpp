// Andrew Naplavkov

#ifndef BOAT_GEOMETRY_SLIPPY_HPP
#define BOAT_GEOMETRY_SLIPPY_HPP

#include <boat/detail/algorithm.hpp>
#include <boat/geometry/transform.hpp>
#include <queue>
#include <unordered_set>

namespace boat::geometry::slippy {
namespace detail {

inline int from_lon(double lon, int z)
{
    return static_cast<int>((lon + 180) / 360 * pow2(z));
}

inline double to_lon(int x, int z)
{
    return static_cast<double>(x) / pow2(z) * 360 - 180;
}

inline int from_lat(double lat, int z)
{
    auto v = std::asinh(std::tan(lat * numbers::degree));
    return static_cast<int>((1 - v / numbers::pi) * pow2(z - 1));
}

inline double to_lat(int y, int z)
{
    auto v = (1 - static_cast<double>(y) / pow2(z - 1)) * numbers::pi;
    return std::atan(std::sinh(v)) * numbers::radian;
}

inline tile snap(geographic::point const& p, int z)
{
    return {.z = z, .y = from_lat(p.y(), z), .x = from_lon(p.x(), z)};
}

inline geographic::box envelope(tile const& t)
{
    auto x1 = to_lon(t.x, t.z);
    auto x2 = to_lon(t.x + 1, t.z);
    auto y1 = to_lat(t.y + 1, t.z);
    auto y2 = to_lat(t.y, t.z);
    return {{std::nexttoward(x1, x2), std::nexttoward(y1, y2)},
            {std::nexttoward(x2, x1), std::nexttoward(y2, y1)}};
}

}  // namespace detail

constexpr int epsg = 3857;
constexpr int zmax = 19;

inline auto covers(geographic::grid const& grid, double resolution)
    -> std::unordered_set<tile>
{
    static constexpr auto d = {-1, 0, 1};
    auto lat_num = 0., lat_den = 0.;
    for (auto& p : grid | std::views::values | std::views::join)
        lat_num += p.y(), lat_den += 1;
    auto ret = std::unordered_set<tile>{};
    if (!lat_den)
        return ret;
    auto scale = std::cos(lat_num / lat_den * numbers::degree) *
                 numbers::earth::equator / pow2(zmax) / tile::size / resolution;
    auto z = iexp2(scale, zmax);
    auto n = pow2(z);
    auto meters = 1.17 * std::begin(grid)->first;
    for (auto& p : grid | std::views::values | std::views::join) {
        auto processed = std::unordered_set{{detail::snap(p, z)}};
        auto queue = std::queue{std::from_range, processed};
        while (!queue.empty()) {
            auto top = queue.front();
            queue.pop();
            ret.insert(top);
            for (auto [dx, dy] : std::views::cartesian_product(d, d)) {
                auto [y, mir] = mirrored_clamp(top.y + dy, 0, n);
                auto x = circular_clamp(top.x + dx + (mir ? n / 2 : 0), 0, n);
                auto t = tile{.z = z, .y = y, .x = x};
                if (!processed.insert(t).second)
                    continue;
                for (auto c : boost::geometry::box_view(detail::envelope(t)))
                    if (meters > boost::geometry::distance(p, c)) {
                        queue.push(t);
                        break;
                    }
            }
        }
    }
    return ret;
}

inline matrix affine(tile const& t)
{
    auto pj = boost::geometry::srs::projection<
        boost::geometry::srs::static_epsg<epsg>>{};
    auto mbr = *transform(srs_forward(pj))(detail::envelope(t));
    auto map = boost::geometry::strategy::transform::
        map_transformer<double, 2, 2, true, false>{mbr, tile::size, tile::size};
    return boost::qvm::inverse(map.matrix());
}

}  // namespace boat::geometry::slippy

#endif  // BOAT_GEOMETRY_SLIPPY_HPP
