// Andrew Naplavkov

#ifndef BOAT_GUI_GEOMETRY_HPP
#define BOAT_GUI_GEOMETRY_HPP

#include <boat/geometry/algorithm.hpp>

namespace boat::gui {

auto recursive_fn(auto... fn)
{
    return overloaded{
        fn...,
        [](this auto&& self, geometry::multi auto&& v) -> void {
            std::ranges::for_each(v, self);
        },
        [](this auto&& self, geometry::dynamic auto&& v) -> void {
            std::visit(self, v);
        }};
}

inline auto multi_point(int width, int height)
{
    auto size = std::max<>(width, height);
    auto num_border_points =
        static_cast<int>(4 * std::max<>(std::sqrt(size), 1.));
    auto num_area_points = std::max<>(size / 4, 1);
    auto mbr = geometry::geographic::box{{}, {width * 1., height * 1.}};
    auto ret = geometry::box_border_interpolate(mbr, num_border_points);
    ret.append_range(geometry::box_area_interpolate(mbr, num_area_points));
    return ret;
}

inline auto sample(  //
    geometry::geographic::geometry_collection geoms,
    geometry::cartesian::box const& box,
    size_t cells_per_side)
{
    namespace bg = boost::geometry;
    namespace geo = geometry;
    using collection = geo::geographic::geometry_collection;
    auto cells =
        std::vector<collection::value_type*>(cells_per_side * cells_per_side);
    auto a = box.min_corner(), b = box.max_corner();
    auto center = bg::return_centroid<geo::cartesian::point>(box);
    auto cell = [&](auto v, auto lo, auto hi) {
        return int(std::clamp(
            cells_per_side * (v - lo) / (hi - lo), 0., cells_per_side - 1.));
    };
    for (auto& geom : geoms) {
        double best = INFINITY;
        auto p = center;
        geo::for_each_point(geom, [&](auto&& v) {
            auto q = geo::cartesian{}(v);
            auto d = bg::comparable_distance(q, center);
            if (d < best) {
                best = d;
                p = q;
            }
        });
        if (best < INFINITY) {
            auto& winner = cells[cell(p.y(), a.y(), b.y()) * cells_per_side +
                                 cell(p.x(), a.x(), b.x())];
            if (!winner || bg::num_points(geom) > bg::num_points(*winner))
                winner = &geom;
        }
    }
    auto ret = collection{};
    for (auto* geom : cells)
        if (geom)
            ret.push_back(std::move(*geom));
    return ret;
}

}  // namespace boat::gui

#endif  // BOAT_GUI_GEOMETRY_HPP
