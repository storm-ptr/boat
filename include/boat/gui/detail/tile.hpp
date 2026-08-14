// Andrew Naplavkov

#ifndef BOAT_GUI_TILE_HPP
#define BOAT_GUI_TILE_HPP

#include <boat/geometry/algorithm.hpp>
#include <boat/geometry/transform.hpp>
#include <boat/tile.hpp>

namespace boat::gui {

auto tiles(  //
    geometry::geographic::grid const& grid,
    int width,
    int height,
    geometry::matrix const& affine,
    geometry::srs_params auto const& crs)
{
    namespace geo = geometry;
    static constexpr auto margin = 1.17;
    static constexpr auto d = {-1, 0, 1};
    auto ret = std::unordered_set<tile>{};
    auto tf = geo::transformation(crs);
    auto fwd = geo::transform(geo::srs_forward(tf), geo::mat_inverse(affine));
    auto inv = geo::transform(geo::mat_forward(affine), geo::srs_inverse(tf));
    auto a = inv(geo::geographic::point(width * .5, height * .5));
    auto b = inv(geo::geographic::point(width * .5 + 1., height * .5 + 1.));
    if (!a || !b || grid.empty())
        return ret;
    auto meters = std::begin(grid)->first;
    auto scale = distance(*a, *b) * numbers::inv_sqrt_2 / meters * tile::size;
    auto z = tile::zoom(width, height, scale);
    auto k = 1. / tile::scale(width, height, z) / tile::size;
    auto snap = [k](auto v) { return static_cast<int>(k * v); };
    for (auto& a : grid | std::views::values | std::views::join) {
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
                auto t = tile{.z = z, .y = top.y + dy, .x = top.x + dx};
                auto [x, y, w, h] = t.rect(width, height);
                if (!w || !h || !done.insert(t).second)
                    continue;
                auto xs = {x, x + w}, ys = {y, y + h};
                for (auto [x, y] : std::views::cartesian_product(xs, ys))
                    if ((b = inv(geo::geographic::point(x, y))) &&
                        margin * meters > distance(a, *b)) {
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
