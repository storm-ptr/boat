// Andrew Naplavkov

#ifndef BOAT_GUI_RASTER_HPP
#define BOAT_GUI_RASTER_HPP

#include <boat/geometry/raster.hpp>
#include <boost/gil.hpp>

namespace boat::gui {

std::optional<std::array<int, 4>> draw_image(  //
    execution_policy auto policy,
    boost::gil::rgba8c_view_t in,
    geometry::matrix const& in_affine,
    geometry::srs_spec auto const& in_crs,
    geometry::matrix const& out_affine,
    geometry::srs_spec auto const& out_crs,
    std::array<int, 4> const& out_rect,
    auto make_writer)
{
    auto [fwd, inv] = geometry::bidirectional_transform(
        in_affine, in_crs, out_affine, out_crs);
    auto in_box =
        geometry::bounds<geometry::geographic::box>(in.width(), in.height());
    auto num_points = std::max<>({in.width(), in.height(), 4z}) / 4;
    auto points = geometry::box_edge_interpolate(
        in_box, size_t(2 * std::sqrt(num_points)) - 1);
    points.append_range(geometry::box_area_interpolate(in_box, num_points));
    auto out_box = fwd(std::move(points))
                       .transform(geometry::minmax)
                       .transform(geometry::cartesian{});
    auto clip_box = geometry::cartesian::box{
        {out_rect[0] * 1., out_rect[1] * 1.},
        {out_rect[0] * 1. + out_rect[2], out_rect[1] * 1. + out_rect[3]}};
    if (!out_box ||
        !boost::geometry::intersection(*out_box, clip_box, *out_box))
        return {};
    auto a = out_box->min_corner(), b = out_box->max_corner();
    auto x0 = int(std::floor(a.x())), y0 = int(std::floor(a.y()));
    auto w = int(std::ceil(b.x())) - x0, h = int(std::ceil(b.y())) - y0;
    if (w <= 0 || h <= 0)
        return {};
    auto write = make_writer(w, h);
    auto ys = std::views::iota(0, h);
    std::for_each(policy, ys.begin(), ys.end(), [&](int y) {
        for (int x{}; x < w; ++x) {
            auto rgba = boost::gil::rgba8_pixel_t{0};
            if (auto p = inv(geometry::geographic::point(x + x0, y + y0));
                p && p->x() >= 0 && p->x() < in.width() && p->y() >= 0 &&
                p->y() < in.height())
                rgba = in(std::ptrdiff_t(p->x()), std::ptrdiff_t(p->y()));
            write(size_t(y) * w + x, rgba);
        }
    });
    return std::array{x0, y0, w, h};
}

}  // namespace boat::gui

#endif  // BOAT_GUI_RASTER_HPP
