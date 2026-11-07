// Andrew Naplavkov

#ifndef BOAT_GUI_QT_HPP
#define BOAT_GUI_QT_HPP

#include <QImage>
#include <QPainter>
#include <QPainterPath>
#include <boat/gui/detail/geometry.hpp>
#include <boat/gui/detail/raster.hpp>

namespace boat::gui {
namespace detail {

constexpr auto qt = overloaded{
    [](geometry::point auto&& v) { return QPointF(v.x(), v.y()); },
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
    return geometry::visitor(
        [&](geometry::point auto&& v) { out.drawPoint(detail::qt(v)); },
        [&](geometry::linestring auto&& v) { out.drawPolyline(detail::qt(v)); },
        [&](geometry::polygon auto&& v) { out.drawPath(detail::qt(v)); });
}

/// Affine matrices map pixels to spatial coordinates.
void draw_image(
    execution_policy auto policy,
    boost::gil::rgba8c_view_t in,
    geometry::matrix const& in_affine,
    geometry::srs_spec auto&& in_crs,
    QPainter& out,
    geometry::matrix const& out_affine,
    geometry::srs_spec auto&& out_crs)
{
    auto img = QImage{};
    auto wnd = out.window();
    auto rect = draw_image(
        policy,
        in,
        in_affine,
        in_crs,
        out_affine,
        out_crs,
        {wnd.x(), wnd.y(), wnd.width(), wnd.height()},
        [&](int w, int h) {
            img = QImage{w, h, QImage::Format_RGBA8888};
            return [bits = reinterpret_cast<boost::gil::rgba8_pixel_t*>(
                        img.bits())](auto i, auto px) { bits[i] = px; };
        });
    if (rect)
        out.drawImage((*rect)[0], (*rect)[1], img);
}

}  // namespace boat::gui

#endif  // BOAT_GUI_QT_HPP
