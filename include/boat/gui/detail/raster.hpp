// Andrew Naplavkov

#ifndef BOAT_GUI_RASTER_HPP
#define BOAT_GUI_RASTER_HPP

#include <boat/geometry/raster.hpp>
#include <boost/gil.hpp>

namespace boat::gui {

std::optional<geometry::raster::box> draw_image(
    execution_policy auto policy,
    boost::gil::rgba8c_view_t in,
    geometry::matrix const& in_affine,
    geometry::srs_spec auto const& in_crs,
    geometry::matrix const& out_affine,
    geometry::srs_spec auto const& out_crs,
    geometry::raster::box const& out_rect,
    auto make_writer)
{
    namespace geo = boat::geometry;
    auto tf = geo::transformation(in_crs, out_crs);
    auto fwd = geo::transform(
        geo::forward(in_affine), geo::forward(tf), geo::backward(out_affine));
    auto inv = geo::transform(
        geo::forward(out_affine), geo::backward(tf), geo::backward(in_affine));
    auto iw = int(in.width()), ih = int(in.height());
    auto in_box = geo::geographic::box{{0., 0.}, {double(iw), double(ih)}};
    auto num_points = std::max<>({iw, ih, 4}) / 4;
    auto points = geo::box_interpolate(
        in_box, size_t(2 * std::sqrt(num_points)), num_points);
    auto out_box = fwd(std::move(points)).transform([](auto const& points) {
        auto ret =
            geo::cartesian::box{{INFINITY, INFINITY}, {-INFINITY, -INFINITY}};
        for (auto const& p : points)
            boost::geometry::expand(ret, geo::cartesian{}(p));
        return ret;
    });
    auto clipped = geo::cartesian::box{};
    if (!out_box || !boost::geometry::intersection(*out_box, out_rect, clipped))
        return {};
    auto a = clipped.min_corner(), b = clipped.max_corner();
    auto r = geo::raster::box{
        {int(std::floor(a.x())), int(std::floor(a.y()))},
        {int(std::ceil(b.x())), int(std::ceil(b.y()))}};
    auto [x0, y0, w, h] = geo::xywh(r);
    if (w <= 0 || h <= 0)
        return {};
    auto write = make_writer(w, h);
    auto ys = std::views::iota(0, h);
    std::for_each(policy, ys.begin(), ys.end(), [&](int y) {
        for (int x{}; x < w; ++x) {
            auto rgba = boost::gil::rgba8_pixel_t{0};
            if (auto p = inv(geo::geographic::point(x + x0, y + y0));
                p && p->x() >= 0 && p->y() >= 0 && p->x() < iw && p->y() < ih)
                rgba = in(std::ptrdiff_t(p->x()), std::ptrdiff_t(p->y()));
            write(size_t(y) * w + x, rgba);
        }
    });
    return r;
}

}  // namespace boat::gui

#endif  // BOAT_GUI_RASTER_HPP
