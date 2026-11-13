// Andrew Naplavkov

#ifndef BOAT_GUI_TILE_HPP
#define BOAT_GUI_TILE_HPP

#include <boat/geometry/raster.hpp>
#include <boat/tile.hpp>

namespace boat::gui {

auto tiles(
    geometry::geographic::multi_point const& points,
    double meters,
    geometry::raster::point ext,
    geometry::matrix const& affine,
    geometry::srs_spec auto const& crs)
{
    namespace geo = geometry;
    static constexpr auto d = {-1, 0, 1};
    auto ret = std::unordered_set<tile>{};
    auto tf = geo::transformation(crs);
    auto fwd = geo::transform(geo::forward(tf), geo::backward(affine));
    auto inv = geo::transform(geo::forward(affine), geo::backward(tf));
    auto cx = ext.x() * .5, cy = ext.y() * .5;
    auto px = inv(geo::geographic::segment{{cx, cy}, {cx + 1., cy + 1.}});
    if (!px || points.empty())
        return ret;
    auto scale = length(*px) * numbers::inv_sqrt_2 / meters * tile::size;
    auto z = tile::zoom(ext, scale);
    auto step = tile::scale(ext, z) * tile::size;
    auto k = 1. / step;
    auto nx = ext.x() / step + (ext.x() % step != 0);
    auto ny = ext.y() / step + (ext.y() % step != 0);
    for (auto& a : points) {
        auto b = fwd(a);
        if (!b)
            continue;
        auto t = tile{.z = z, .y = int(k * b->y()), .x = int(k * b->x())};
        if (!boost::geometry::is_valid(t.rect(ext)))
            continue;
        auto done = std::unordered_set{{t}};
        auto queue = std::queue{std::from_range, done};
        while (!queue.empty()) {
            auto top = queue.front();
            queue.pop();
            ret.insert(top);
            for (auto [dx, dy] : std::views::cartesian_product(d, d)) {
                auto t = tile{
                    .z = z,
                    .y = (top.y + dy + ny) % ny,
                    .x = (top.x + dx + nx) % nx};
                if (!done.insert(t).second)
                    continue;
                for (auto const& p :
                     boost::geometry::box_view{geo::geographic{}(t.rect(ext))} |
                         std::views::take(4))
                    if ((b = inv(p)) && meters > distance(a, *b)) {
                        queue.push(t);
                        break;
                    }
            }
        }
    }
    return ret;
}

}  // namespace boat::gui

#endif  // BOAT_GUI_TILE_HPP
