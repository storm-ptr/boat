// Andrew Naplavkov

#ifndef BOAT_GUI_QT_HPP
#define BOAT_GUI_QT_HPP

#include <QImage>
#include <QPainter>
#include <QPainterPath>
#include <boat/geometry/raster.hpp>
#include <boat/gui/detail/geometry.hpp>
#include <boat/gui/detail/gil.hpp>

namespace boat::gui {
namespace detail {

constexpr auto qt = overloaded{
    [](geometry::point auto&& v) { return QPointF(v.x(), v.y()); },
    [](this auto&& self, geometry::box auto&& v) -> QRectF {
        return {self(v.min_corner()), self(v.max_corner())};
    },
    [](this auto&& self, geometry::curve auto&& v) -> QList<QPointF> {
        return v | std::views::transform(self) | std::ranges::to<QList>();
    },
    [](this auto&& self, geometry::polygon auto&& v) -> QPainterPath {
        auto ret = QPainterPath{};
        ret.addPolygon(self(v.outer()));
        for (auto& item : v.inners())
            ret.addPolygon(self(item));
        return ret;
    }};

}  // namespace detail

inline auto draw_geometry(QPainter& out)
{
    return recursive_fn(
        [&](geometry::point auto&& v) { out.drawPoint(detail::qt(v)); },
        [&](geometry::linestring auto&& v) { out.drawPolyline(detail::qt(v)); },
        [&](geometry::polygon auto&& v) { out.drawPath(detail::qt(v)); });
}

/// Affine matrices map pixels to spatial coordinates.
void draw_image(  //
    execution_policy auto policy,
    boost::gil::rgba8c_view_t in,
    geometry::matrix const& in_affine,
    geometry::srs_spec auto&& in_crs,
    QPainter& out,
    geometry::matrix const& out_affine,
    geometry::srs_spec auto&& out_crs)
{
    auto [fwd, inv] = geometry::bidirectional_transform(
        in_affine, in_crs, out_affine, out_crs);
    auto mbr =
        fwd(multi_point(in.width(), in.height()))
            .transform(geometry::minmax)
            .transform(detail::qt)
            .transform(&QRectF::toAlignedRect)
            .transform(std::bind_front(&QRect::intersected, out.window()));
    if (!mbr || mbr->isEmpty())
        return;
    auto img = QImage{mbr->size(), QImage::Format_RGBA8888};
    auto bits = img.bits();
    auto stride = img.bytesPerLine();
    auto ys = std::views::iota(0, img.height());
    auto pixel = get_pixel(in);
    std::for_each(policy, ys.begin(), ys.end(), [&](int y) {
        auto ln =
            reinterpret_cast<boost::gil::rgba8_pixel_t*>(bits + y * stride);
        for (int x{}; x < img.width(); ++x)
            ln[x] = inv(geometry::geographic::point(x + mbr->x(), y + mbr->y()))
                        .and_then(pixel)
                        .value_or(boost::gil::rgba8_pixel_t{0});
    });
    out.drawImage(mbr->topLeft(), img);
}

}  // namespace boat::gui

#endif  // BOAT_GUI_QT_HPP
