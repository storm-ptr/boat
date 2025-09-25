// Andrew Naplavkov

#ifndef BOAT_SLIPPY_HPP
#define BOAT_SLIPPY_HPP

#include <boat/detail/utility.hpp>
#include <boat/geometry/model.hpp>
#include <boost/pfr/ops_fields.hpp>
#include <queue>
#include <unordered_set>

namespace boat::slippy {

struct tile {
    int x;
    int y;
    int z;

    friend bool operator==(tile const&, tile const&) = default;
};

}  // namespace boat::slippy

template <>
struct std::hash<boat::slippy::tile> {
    static size_t operator()(boat::slippy::tile const& that)
    {
        return boost::pfr::hash_fields(that);
    }
};

namespace boat::slippy {

static auto const steps = std::views::iota(0, 20) |
                          std::views::transform([](int z) {
                              return 2 * pi * 6'378'137 / std::pow(2, z);
                          }) |
                          std::ranges::to<std::vector>();

inline double step_factor(double lat)
{
    return std::cos(lat * degree);
}

inline int zoom(double step, double lat)
{
    auto k = step_factor(lat) * 1.67;
    for (auto z = static_cast<int>(steps.size()) - 1; z > 0; --z)
        if (step < steps[z] * k)
            return z;
    return 0;
}

inline int lon2tilex(double lon, int z)
{
    return static_cast<int>((lon + 180) / 360 * std::pow(2, z));
}

inline int lat2tiley(double lat, int z)
{
    return static_cast<int>((1 - std::asinh(std::tan(lat * degree)) / pi) *
                            std::pow(2, z - 1));
}

inline double tilex2lon(int x, int z)
{
    return x / std::pow(2, z) * 360 - 180;
}

inline double tiley2lat(int y, int z)
{
    auto n = pi * (1 - y / std::pow(2, z - 1));
    return std::atan((std::exp(n) - std::exp(-n)) / 2) * radian;
}

inline tile to_tile(geometry::geographic::point const& ll, int z)
{
    return {lon2tilex(ll.x(), z), lat2tiley(ll.y(), z), z};
}

inline tile corrected(tile const& t)
{
    int n = static_cast<int>(std::pow(2, t.z));
    return {(t.x < 0 ? n : 0) + t.x % n, (t.y < 0 ? n : 0) + t.y % n, t.z};
}

inline geometry::geographic::box envelope(tile const& t)
{
    return {{tilex2lon(t.x, t.z), tiley2lat(t.y + 1, t.z)},
            {tilex2lon(t.x + 1, t.z), tiley2lat(t.y, t.z)}};
}

inline std::unordered_set<tile> to_tiles(geometry::geographic::grid const& grid,
                                         double resolution)
{
    static constexpr auto d = {-1, 0, 1};
    auto ret = std::unordered_set<tile>{};
    auto lat_num = 0., lat_denom = 0.;
    for (auto& ll : grid | std::views::values | std::views::join)
        lat_num += ll.y(), lat_denom += 1;
    if (!lat_denom)
        return ret;
    auto step = resolution * 256;
    auto z = zoom(step, lat_num / lat_denom);
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

#endif  // BOAT_SLIPPY_HPP
