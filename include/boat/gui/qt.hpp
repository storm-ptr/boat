// Andrew Naplavkov

#ifndef BOAT_GUI_QT_HPP
#define BOAT_GUI_QT_HPP

#include <QImage>
#include <QPainter>
#include <QPainterPath>
#include <boat/gui/detail/geometry.hpp>
#include <boat/gui/detail/gil.hpp>
#include <execution>

namespace boat::gui {

constexpr auto to_qt = overloaded{
    [](geometry::point auto&& g) { return QPointF(g.x(), g.y()); },
    [](this auto&& self, geometry::box auto&& g) -> QRectF {
        return {self(g.min_corner()), self(g.max_corner())};
    },
    [](this auto&& self, geometry::curve auto&& g) -> QList<QPointF> {
        return g | std::views::transform(self) | std::ranges::to<QList>();
    },
    [](this auto&& self, geometry::polygon auto&& g) -> QPainterPath {
        auto ret = QPainterPath{};
        ret.addPolygon(self(g.outer()));
        for (auto& item : g.inners())
            ret.addPolygon(self(item));
        return ret;
    }};

inline auto draw_geometry(QPainter& out)
{
    return overloaded{
        [&](geometry::point auto&& g) { out.drawPoint(to_qt(g)); },
        [&](geometry::linestring auto&& g) { out.drawPolyline(to_qt(g)); },
        [&](geometry::polygon auto&& g) { out.drawPath(to_qt(g)); },
        [](this auto&& self, geometry::multi auto&& g) -> void {
            std::ranges::for_each(g, self);
        },
        [](this auto&& self, geometry::dynamic auto&& g) -> void {
            std::visit(self, g);
        }};
}

void draw_image(  //
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
    auto img = QImage{mbr->size(), QImage::Format_ARGB32_Premultiplied};
    auto ys = std::views::iota(0, img.height());
    auto pixel = get_pixel(in);
    std::for_each(std::execution::par, ys.begin(), ys.end(), [&](int y) {
        auto rgb = reinterpret_cast<QRgb*>(img.scanLine(y));
        for (int x{}; x < img.width(); ++x)
            rgb[x] =
                inv(geometry::geographic::point(x + mbr->x(), y + mbr->y()))
                    .and_then(pixel)
                    .transform([](auto const& px) {
                        return qRgba(get_color(px, boost::gil::red_t()),
                                     get_color(px, boost::gil::green_t()),
                                     get_color(px, boost::gil::blue_t()),
                                     get_color(px, boost::gil::alpha_t()));
                    })
                    .value_or(Qt::transparent);
    });
    out.drawImage(mbr->topLeft(), img);
}

}  // namespace boat::gui

#endif  // BOAT_GUI_QT_HPP
