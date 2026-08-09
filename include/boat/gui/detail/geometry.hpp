// Andrew Naplavkov

#ifndef BOAT_GUI_GEOMETRY_HPP
#define BOAT_GUI_GEOMETRY_HPP

#include <boat/geometry/raster.hpp>

namespace boat::gui {

auto bidirectional(  //
    geometry::matrix const& affine1,
    geometry::srs_params auto const& crs1,
    geometry::matrix const& affine2,
    geometry::srs_params auto const& crs2)
{
    auto tf = geometry::srs::transformation<>(crs1, crs2);
    return std::pair{
        geometry::transform(  //
            geometry::mat_forward(affine1),
            geometry::srs_forward(tf),
            geometry::mat_inverse(affine2)),
        geometry::transform(  //
            geometry::mat_forward(affine2),
            geometry::srs_inverse(tf),
            geometry::mat_inverse(affine1)),
    };
}

auto boxes(  //
    geometry::geographic::grid const& grid,
    geometry::srs_params auto const& crs)
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

inline auto multi_point(int width, int height)
{
    auto hi = std::max<>(width, height);
    auto num_inners = std::max<>(hi / 4, 1);
    auto num_per_edge = std::max<>(std::sqrt(hi), 1.);
    auto ret = geometry::box_interpolate<geometry::geographic::multi_point>(
        width, height, num_inners);
    auto mbr = geometry::geographic::box{{}, {width * 1., height * 1.}};
    for (auto tup : boost::geometry::box_view{mbr} | std::views::pairwise) {
        auto a = std::get<0>(tup), b = std::get<1>(tup);
        ret.push_back(a);
        ret.append_range(
            std::views::iota(0, static_cast<int>(num_per_edge)) |
            std::views::transform([&](auto n) -> geometry::geographic::point {
                auto t = (n + 1.) / (num_per_edge + 1.);
                return {std::lerp(a.x(), b.x(), t), std::lerp(a.y(), b.y(), t)};
            }));
    }
    return ret;
}

}  // namespace boat::gui

#endif  // BOAT_GUI_GEOMETRY_HPP
