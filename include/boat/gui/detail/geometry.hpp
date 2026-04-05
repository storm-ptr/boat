// Andrew Naplavkov

#ifndef BOAT_GUI_GEOMETRY_HPP
#define BOAT_GUI_GEOMETRY_HPP

#include <boat/geometry/raster.hpp>
#include <boat/geometry/transform.hpp>
#include <boat/geometry/wkb.hpp>

namespace boat::gui {

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
            std::views::transform([&](auto i) -> geometry::geographic::point {
                auto t = (i + 1.) / (num_per_edge + 1.);
                return {std::lerp(a.x(), b.x(), t), std::lerp(a.y(), b.y(), t)};
            }));
    }
    return ret;
}

inline auto epsg(int code)
{
    return boost::geometry::srs::epsg{code};
}

auto forward(  //
    geometry::srs_spec auto const& sys1,
    geometry::matrix const& affine2,
    geometry::srs_spec auto const& sys2)
{
    auto tf = boost::geometry::srs::transformation<>{sys1, sys2};
    return geometry::transform(  //
        geometry::srs_forward(tf),
        geometry::mat_inverse(affine2));
}

auto bidirectional(  //
    geometry::matrix const& affine1,
    geometry::srs_spec auto const& sys1,
    geometry::matrix const& affine2,
    geometry::srs_spec auto const& sys2)
{
    auto tf = boost::geometry::srs::transformation<>{sys1, sys2};
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

inline auto variant(blob_view wkb)
{
    auto ret = geometry::geographic::variant{};
    wkb >> ret;
    return ret;
}

auto boxes(  //
    geometry::geographic::grid const& grid,
    geometry::srs_spec auto const& sys)
{
    static auto const antimeridian = geometry::geographic::linestring{
        {-180., 90.}, {-180., 0.}, {-180., -90.}};
    auto lls = std::vector<geometry::geographic::box>{};
    for (auto& lvl : grid | std::views::reverse) {
        auto r = lvl.first * numbers::inv_sqrt_2;
        if (r >= numbers::earth::sqrt_area / 4)
            continue;
        for (auto buf = geometry::buffer(r, 16); auto& p : lvl.second)
            if (auto poly = buf(p); r < distance(p, antimeridian))
                lls.push_back(geometry::minmax(poly));
            else {
                auto ps = geometry::geographic::multi_point{};
                ps.assign_range(  //
                    poly.outer() |
                    std::views::filter([](auto& p) { return p.x() < 0.; }));
                lls.push_back(geometry::minmax(ps));
                ps.assign_range(  //
                    poly.outer() |
                    std::views::filter([](auto& p) { return p.x() > 0.; }));
                lls.push_back(geometry::minmax(ps));
            }
    }
    auto ret = std::vector<geometry::cartesian::box>{};
    auto fwd = geometry::transform(
        geometry::srs_forward(geometry::transformation(sys)));
    for (auto& ll : lls)
        if (auto xy = fwd(ll).transform(geometry::cartesian{}))
            ret.push_back(*xy);
    return ret;
}

}  // namespace boat::gui

#endif  // BOAT_GUI_GEOMETRY_HPP
