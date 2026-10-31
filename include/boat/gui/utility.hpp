// Andrew Naplavkov

#ifndef BOAT_GUI_UTILITY_HPP
#define BOAT_GUI_UTILITY_HPP

#include <boat/geometry/raster.hpp>
#include <boat/gui/scene.hpp>

namespace boat::gui {

geometry::matrix affine(int width, int height, view const& v, auto const& crs)
{
    if constexpr (geometry::projection_or_transformation<decltype(crs)>)
        return affine(
            width, height, v, geometry::transform(geometry::srs_forward(crs)));
    else if constexpr (geometry::srs_spec<decltype(crs)>)
        return affine(width, height, v, geometry::transformation(crs));
    else {
        auto a = geometry::cartesian{}(*crs(v.center));
        auto b = geometry::cartesian{}(
            *crs(geometry::add_meters(v.center, v.resolution, 0.)));
        return geometry::affine(width, height, {a, b});
    }
}

auto boxes(  //
    geometry::geographic::grid const& grid,
    geometry::srs_spec auto const& crs)
{
    auto ret = std::vector<geometry::cartesian::box>{};
    auto fwd = geometry::transform(
        geometry::srs_forward(geometry::transformation(crs)));
    auto add = [&](geometry::geographic::box const& ll) {
        auto a = ll.min_corner(), b = ll.max_corner();
        if (a.x() <= -180. || b.x() >= 180. || a.y() <= -90. || b.y() >= 90.)
            return;
        if (auto xy = fwd(ll).transform(geometry::cartesian{}))
            ret.push_back(*xy);
    };
    for (auto& lvl : grid | std::views::reverse) {
        auto d = lvl.first * numbers::inv_sqrt_2;
        if (d >= numbers::earth::sqrt_area / 4)
            continue;
        for (auto& p : lvl.second) {
            auto x = p.x(), y = p.y();
            auto dx = d * geometry::meter(y), dy = d * geometry::meter();
            add({{x, y}, {x + dx, y + dy}});
            add({{x - dx, y}, {x, y + dy}});
            add({{x, y - dy}, {x + dx, y}});
            add({{x - dx, y - dy}, {x, y}});
        }
    }
    return ret;
}

}  // namespace boat::gui

#endif  // BOAT_GUI_UTILITY_HPP
