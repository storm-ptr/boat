// Andrew Naplavkov

#ifndef BOAT_GUI_TILE_HPP
#define BOAT_GUI_TILE_HPP

#include <boat/geometry/algorithm.hpp>
#include <boat/geometry/transform.hpp>
#include <boat/tile.hpp>

namespace boat::gui {

std::unordered_set<tile> tiles(  //
    int width,
    int height,
    geometry::matrix const& mat,
    geometry::srs_spec auto const& sys,
    geometry::geographic::grid const& grid)
{
    namespace bg = boost::geometry;
    static constexpr auto margin = 1.17;
    static constexpr auto d = {-1, 0, 1};

    auto tf = geometry::transformation(sys);
    auto fwd = geometry::transform(  //
        geometry::srs_forward(tf),
        geometry::mat_inverse(mat));
    auto inv = geometry::transform(  //
        geometry::mat_forward(mat),
        geometry::srs_inverse(tf));

    auto scale_num = 0., scale_den = 0.;
    for (auto& a : grid | std::views::values | std::views::join) {
        auto b = fwd(a);
        if (!b || !(b = inv(geometry::add_value(*b, 1))))
            continue;
        scale_num += bg::distance(a, *b) * numbers::inv_sqrt_2;
        scale_den += 1.;
    }
    auto ret = std::unordered_set<tile>{};
    if (!scale_den)
        return ret;
    auto meters = std::begin(grid)->first;
    auto scale = scale_num / scale_den / meters * tile::size;

    auto t = tile{.z = tile::zoom(width, height, scale * margin)};
    auto k = 1. / tile::scale(width, height, t.z) / tile::size;
    auto snap = [k](auto v) { return static_cast<int>(k * v); };
    auto xmax = snap(width - 1), ymax = snap(height - 1);
    for (auto& a : grid | std::views::values | std::views::join) {
        auto b = fwd(a);
        if (!b)
            continue;
        t.x = snap(b->x()), t.y = snap(b->y());
        if (!between(t.x, 0, xmax) || !between(t.y, 0, ymax))
            continue;
        auto done = std::unordered_set{{t}};
        auto queue = std::queue{std::from_range, done};
        while (!queue.empty()) {
            auto top = queue.front();
            queue.pop();
            ret.insert(top);
            for (auto [dx, dy] : std::views::cartesian_product(d, d)) {
                t.x = top.x + dx, t.y = top.y + dy;
                if (!between(t.x, 0, xmax) || !between(t.y, 0, ymax) ||
                    !done.insert(t).second)
                    continue;
                auto [x1, y1] = t.min_corner(width, height);
                auto [x2, y2] = t.max_corner(width, height);
                auto xs = {x1, x2};
                auto ys = {y1, y2};
                for (auto [x, y] : std::views::cartesian_product(xs, ys))
                    if ((b = inv(geometry::geographic::point(x, y))) &&
                        margin * meters > bg::distance(a, *b)) {
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
