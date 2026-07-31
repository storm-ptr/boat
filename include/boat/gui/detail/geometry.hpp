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
    static auto const antimeridian = geometry::geographic::linestring{
        {-180., 90.}, {-180., 0.}, {-180., -90.}};
    auto lls = std::vector<geometry::geographic::box>{};
    for (auto& lvl : grid | std::views::reverse) {
        auto r = lvl.first * numbers::inv_sqrt_2;
        if (r >= numbers::earth::sqrt_area / 4)
            continue;
        auto d = {-r, r};
        for (auto& a : lvl.second) {
            auto asafe = distance(a, antimeridian) > r;
            for (auto [dx, dy] : std::views::cartesian_product(d, d)) {
                auto b = geometry::add_meters(a, dx, dy);
                auto bsafe = distance(b, antimeridian) > r;
                if (asafe || bsafe)
                    lls.push_back(geometry::minmax(
                        geometry::geographic::multi_point{a, b}));
            }
        }
    }
    auto ret = std::vector<geometry::cartesian::box>{};
    auto fwd = geometry::transform(
        geometry::srs_forward(geometry::transformation(crs)));
    for (auto& ll : lls)
        if (auto xy = fwd(ll).transform(geometry::cartesian{}))
            ret.push_back(*xy);
    return ret;
}

inline auto multi_point(int width, int height)
{
    constexpr auto num_per_edge = 17;
    constexpr auto num_inners = 71;
    auto ret = geometry::box_interpolate<geometry::geographic::multi_point>(
        width, height, num_inners);
    auto mbr = geometry::geographic::box{{}, {width * 1., height * 1.}};
    for (auto tuple : boost::geometry::box_view{mbr} | std::views::pairwise) {
        auto a = std::get<0>(tuple), b = std::get<1>(tuple);
        ret.push_back(a);
        ret.append_range(
            std::views::iota(0, num_per_edge) |
            std::views::transform([&](auto n) -> geometry::geographic::point {
                auto t = (n + 1.) / (num_per_edge + 1.);
                return {std::lerp(a.x(), b.x(), t), std::lerp(a.y(), b.y(), t)};
            }));
    }
    return ret;
}

}  // namespace boat::gui

#endif  // BOAT_GUI_GEOMETRY_HPP
