// Andrew Naplavkov

#ifndef BOAT_GEOMETRY_SLIPPY_HPP
#define BOAT_GEOMETRY_SLIPPY_HPP

#include <boat/geometry/transform.hpp>
#include <queue>
#include <unordered_set>

namespace boat::geometry::slippy {

constexpr auto epsg = 3857;
constexpr auto num_pixels = 256;
constexpr auto num_zooms = 20;

namespace detail {

inline int lon2tilex(double lon, int z)
{
    return static_cast<int>((lon + 180) / 360 * std::pow(2, z));
}

inline int lat2tiley(double lat, int z)
{
    return static_cast<int>(
        (1 - std::asinh(std::tan(lat * numbers::deg)) / numbers::pi) *
        std::pow(2, z - 1));
}

inline double tilex2lon(int x, int z)
{
    return x / std::pow(2, z) * 360 - 180;
}

inline double tiley2lat(int y, int z)
{
    auto n = numbers::pi * (1 - y / std::pow(2, z - 1));
    return std::atan((std::exp(n) - std::exp(-n)) / 2) * numbers::rad;
}

inline double equatorial_resolution(int z)
{
    return 2 * numbers::pi * numbers::earth::equatorial_radius /
           std::pow(2, z) / num_pixels;
}

inline int zoom(double resolution, double lat)
{
    auto k = 1.67 * std::cos(lat * numbers::deg);
    for (int z = num_zooms - 1; z > 0; --z)
        if (resolution < k * equatorial_resolution(z))
            return z;
    return 0;
}

}  // namespace detail

inline geographic::box minmax(tile const& t)
{
    auto x1 = detail::tilex2lon(t.x, t.z);
    auto x2 = detail::tilex2lon(t.x + 1, t.z);
    auto y1 = detail::tiley2lat(t.y + 1, t.z);
    auto y2 = detail::tiley2lat(t.y, t.z);
    return {{std::nexttoward(x1, x2), std::nexttoward(y1, y2)},
            {std::nexttoward(x2, x1), std::nexttoward(y2, y1)}};
}

inline matrix to_matrix(tile const& t)
{
    auto pj = boost::geometry::srs::projection<
        boost::geometry::srs::static_epsg<epsg>>{};
    auto mbr = *transform(srs_forward(pj))(minmax(t));
    auto inv = boost::geometry::strategy::transform::
        map_transformer<double, 2, 2, true, false>{mbr, num_pixels, num_pixels};
    return boost::qvm::inverse(inv.matrix());
}

inline tile to_tile(geographic::point const& p, int z)
{
    return {detail::lon2tilex(p.x(), z), detail::lat2tiley(p.y(), z), z};
}

inline std::unordered_set<tile> coverage(geographic::grid const& grid,
                                         double resolution)
{
    static constexpr auto d = {-1, 0, 1};
    auto ret = std::unordered_set<tile>{};
    auto lat_num = 0., lat_denom = 0.;
    for (auto& p : grid | std::views::values | std::views::join)
        lat_num += p.y(), lat_denom += 1;
    if (!lat_denom)
        return ret;
    auto z = detail::zoom(resolution, lat_num / lat_denom);
    auto n = static_cast<int>(std::pow(2, z));
    auto step = 1.051 * std::begin(grid)->first;
    for (auto& p : grid | std::views::values | std::views::join) {
        auto processed = std::unordered_set{{to_tile(p, z)}};
        auto queue = std::queue{std::from_range, processed};
        while (!queue.empty()) {
            auto top = queue.front();
            queue.pop();
            ret.insert(top);
            for (auto [dx, dy] : std::views::cartesian_product(d, d)) {
                auto [y, mir] = mirrored_clamp(top.y + dy, 0, n);
                auto x = circular_clamp(top.x + dx + (mir ? n / 2 : 0), 0, n);
                auto t = tile{x, y, z};
                if (!processed.insert(t).second)
                    continue;
                for (auto c : boost::geometry::box_view(minmax(t)))
                    if (boost::geometry::distance(p, c) < step) {
                        queue.push(t);
                        break;
                    }
            }
        }
    }
    return ret;
}

}  // namespace boat::geometry::slippy

#endif  // BOAT_GEOMETRY_SLIPPY_HPP
