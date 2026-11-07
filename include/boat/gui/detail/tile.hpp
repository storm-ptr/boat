// Andrew Naplavkov

#ifndef BOAT_GUI_TILE_HPP
#define BOAT_GUI_TILE_HPP

#include <boat/geometry/raster.hpp>
#include <boat/tile.hpp>

namespace boat::gui {

auto tiles(  //
    geometry::geographic::grid const& grid,
    int width,
    int height,
    geometry::matrix const& affine,
    geometry::srs_spec auto const& crs)
{
    namespace geo = geometry;
    static constexpr auto d = {-1, 0, 1};
    auto ret = std::unordered_set<tile>{};
    auto [fwd, inv] = geo::bidirectional_transform(affine, crs);
    auto px = inv(geo::pixel<geo::geographic::segment>(width, height));
    if (!px || grid.empty())
        return ret;
    auto meters = std::begin(grid)->first;
    auto scale = length(*px) * numbers::inv_sqrt_2 / meters * tile::size;
    auto z = tile::zoom(width, height, scale);
    auto k = 1. / tile::scale(width, height, z) / tile::size;
    auto snap = [k](auto v) { return static_cast<int>(k * v); };
    auto nx = static_cast<int>(std::ceil(width * k));
    auto ny = static_cast<int>(std::ceil(height * k));
    for (auto& a : grid.begin()->second) {
        auto b = fwd(a);
        if (!b)
            continue;
        auto t = tile{.z = z, .y = snap(b->y()), .x = snap(b->x())};
        if (auto [x, y, w, h] = t.rect(width, height); !w || !h)
            continue;
        auto done = std::unordered_set{{t}};
        auto queue = std::queue{std::from_range, done};
        while (!queue.empty()) {
            auto top = queue.front();
            queue.pop();
            ret.insert(top);
            for (auto [dx, dy] : std::views::cartesian_product(d, d)) {
                auto t = tile{.z = z,
                              .y = (top.y + dy + ny) % ny,
                              .x = (top.x + dx + nx) % nx};
                auto [x, y, w, h] = t.rect(width, height);
                if (!w || !h || !done.insert(t).second)
                    continue;
                auto xs = {x, x + w}, ys = {y, y + h};
                for (auto [x, y] : std::views::cartesian_product(xs, ys))
                    if ((b = inv(geo::geographic::point(x, y))) &&
                        meters > distance(a, *b)) {
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
