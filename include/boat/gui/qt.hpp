// Andrew Naplavkov

#ifndef BOAT_GUI_QT_HPP
#define BOAT_GUI_QT_HPP

#include <QImage>
#include <QPainter>
#include <QPainterPath>
#include <boat/gui/detail/geometry.hpp>
#include <boat/gui/detail/gil.hpp>

namespace boat::gui {

constexpr auto to_qt = overloaded{
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

inline auto draw_geometry(QPainter& out)
{
    return overloaded{
        [&](geometry::point auto&& v) { out.drawPoint(to_qt(v)); },
        [&](geometry::linestring auto&& v) { out.drawPolyline(to_qt(v)); },
        [&](geometry::polygon auto&& v) { out.drawPath(to_qt(v)); },
        [](this auto&& self, geometry::multi auto&& v) -> void {
            std::ranges::for_each(v, self);
        },
        [](this auto&& self, geometry::dynamic auto&& v) -> void {
            std::visit(self, v);
        }};
}

void draw_image(  //
    execution_policy auto policy,
    boost::gil::rgba8c_view_t in,
    geometry::matrix const& in_affine,
    geometry::srs_params auto&& in_crs,
    QPainter& out,
    geometry::matrix const& out_affine,
    geometry::srs_params auto&& out_crs)
{
    auto [fwd, inv] = bidirectional(in_affine, in_crs, out_affine, out_crs);
    auto mbr =
        fwd(multi_point(in.width(), in.height()))
            .transform(geometry::minmax)
            .transform(to_qt)
            .transform(&QRectF::toAlignedRect)
            .transform(std::bind_front(&QRect::intersected, out.window()));
    if (!mbr || mbr->isEmpty())
        return;
    auto img = QImage{mbr->size(), QImage::Format_RGBA8888};
    auto ys = std::views::iota(0, img.height());
    auto pixel = get_pixel(in);
    std::for_each(policy, ys.begin(), ys.end(), [&](int y) {
        auto ln = reinterpret_cast<uint8_t*>(img.scanLine(y));
        for (int x{}; x < img.width(); ++x) {
            auto px =
                inv(geometry::geographic::point(x + mbr->x(), y + mbr->y()))
                    .and_then(pixel);
            if (px)
                *reinterpret_cast<boost::gil::rgba8_pixel_t*>(ln + x * 4) = *px;
            else
                std::fill_n(ln + x * 4, 4, 0);
        }
    });
    out.drawImage(mbr->topLeft(), img);
}

}  // namespace boat::gui

#endif  // BOAT_GUI_QT_HPP
