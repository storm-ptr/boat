// Andrew Naplavkov

#ifndef BOAT_GEOMETRY_TILE_HPP
#define BOAT_GEOMETRY_TILE_HPP

#include <boat/detail/algorithm.hpp>
#include <boat/geometry/detail/utility.hpp>
#include <boat/geometry/transform.hpp>

namespace boat::geometry {
namespace detail {

inline int align(int width, int height)
{
    auto size = static_cast<size_t>(std::max<>({width, height, tile::size}));
    return static_cast<int>(std::bit_ceil(size));
}

inline int zmax(int aligned)
{
    auto num_tiles = static_cast<size_t>(aligned / tile::size);
    return static_cast<int>(std::bit_width(num_tiles)) - 1;
}

}  // namespace detail

std::unordered_set<tile> covers(  //
    geographic::grid const& grid,
    double resolution,
    int limit,
    int width,
    int height,
    matrix const& mat,
    srs_spec auto const& srs)
{
    auto tf = transformation(srs);
    auto fwd = transform(srs_forward(tf), mat_inverse(mat));
    auto inv = transform(mat_forward(mat), srs_inverse(tf));
    auto boxes = std::vector<cartesian::box>{};
    auto scale_num = 0., scale_den = 0.;
    for (auto& a : grid | std::views::values | std::views::join) {
        auto b = fwd(a);
        if (!b)
            continue;
        auto c = cast<cartesian::point>(*b);
        if (!(b = inv(add_value(*b, 1))))
            continue;
        auto res = boost::geometry::distance(a, *b) * numbers::inv_sqrt_2;
        if (!res)
            continue;
        scale_num += res / resolution, scale_den += 1.;
        auto d = .5 * grid.begin()->first / res;
        boxes.emplace_back(add_value(c, -d), add_value(c, d));
    }
    auto ret = std::unordered_set<tile>{};
    if (!scale_den)
        return ret;
    auto aligned = detail::align(width, height);
    auto z = iexp2(scale_num / scale_den, detail::zmax(aligned));
    auto k = pow2(z) * 1. / aligned;
    auto snap = [=](point auto const& p) {
        return tile{
            .z = z,
            .y = static_cast<int>(k * std::clamp(p.y(), 0., height - 1.)),
            .x = static_cast<int>(k * std::clamp(p.x(), 0., width - 1.))};
    };
    for (auto& box : boxes) {
        auto a = snap(box.min_corner()), b = snap(box.max_corner());
        for (int x = a.x; x <= b.x; ++x)
            for (int y = a.y; y <= b.y; ++y) {
                ret.insert({.z = z, .y = y, .x = x});
                if (ret.size() >= limit)
                    return ret;
            }
    }
    return ret;
}

inline cartesian::box envelope(int width, int height, tile const& t)
{
    auto k = detail::align(width, height) / pow2(t.z);
    auto x1 = std::clamp(k * t.x, 0, width);
    auto y1 = std::clamp(k * t.y, 0, height);
    auto x2 = std::clamp(k * (t.x + 1), 0, width);
    auto y2 = std::clamp(k * (t.y + 1), 0, height);
    return {{x1 * 1., y1 * 1.}, {x2 * 1., y2 * 1.}};
}

inline int downscaling(int width, int height, int z)
{
    return pow2(detail::zmax(detail::align(width, height)) - z);
}

}  // namespace boat::geometry

#endif  // BOAT_GEOMETRY_TILE_HPP
