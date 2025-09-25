// Andrew Naplavkov

#ifndef BOAT_GUI_QT_HPP
#define BOAT_GUI_QT_HPP

#include <QImage>
#include <QPainter>
#include <QPainterPath>
#include <boat/geometry/transform.hpp>
#include <boat/geometry/wkb.hpp>
#include <boat/gui/feature.hpp>
#include <boost/geometry/srs/epsg.hpp>
#include <execution>

namespace boat::gui::qt {
namespace detail {

constexpr auto adapt = overloaded{
    [](geometry::point auto const& g) { return QPointF(g.x(), g.y()); },
    [](this auto&& self, geometry::box auto const& g) -> QRectF {
        return {self(g.min_corner()), self(g.max_corner())};
    },
    [](this auto&& self, geometry::curve auto const& g) -> QList<QPointF> {
        return g | std::views::transform(self) | std::ranges::to<QList>();
    },
    [](this auto&& self, geometry::polygon auto const& g) -> QPainterPath {
        auto ret = QPainterPath{};
        ret.addPolygon(self(g.outer()));
        for (auto& item : g.inners())
            ret.addPolygon(self(item));
        return ret;
    }};

void draw(geometry::ogc99 auto const& geom, QPainter& paint)
{
    overloaded{
        [&](geometry::point auto& g) { paint.drawPoint(adapt(g)); },
        [&](geometry::linestring auto& g) { paint.drawPolyline(adapt(g)); },
        [&](geometry::polygon auto& g) { paint.drawPath(adapt(g)); },
        [](this auto&& self, geometry::multi auto& g) -> void {
            std::ranges::for_each(g, self);
        },
        [](this auto&& self, geometry::dynamic auto& g) -> void {
            std::visit(self, g);
        }}(geom);
}

}  // namespace detail

void draw(feature const& feat,
          geometry::srs auto const& srs,
          geometry::box auto const& mbr,
          QPainter& paint)
{
    auto wnd = paint.window();
    auto tf = boost::geometry::srs::transformation<>(
        boost::geometry::srs::epsg{feat.epsg}, srs);
    auto fwd = geometry::transformer(
        geometry::forwarder(tf),
        geometry::forwarder(mbr, wnd.width(), wnd.height()));
    auto shape = geometry::geographic::variant{};
    blob_view{feat.shape} >> shape;
    if (feat.raster.empty()) {
        if (auto pj = fwd(shape))
            detail::draw(*pj, paint);
        return;
    }
    auto mbr1 = geometry::envelope(shape);
    auto mbr2 = fwd(mbr1)
                    .transform(detail::adapt)
                    .transform(&QRectF::toAlignedRect)
                    .transform(std::bind_front(&QRect::intersected, wnd));
    auto img1 = QImage{};
    if (!mbr2 || mbr2->isEmpty() ||
        !img1.loadFromData(reinterpret_cast<uchar const*>(feat.raster.data()),
                           static_cast<int>(feat.raster.size())))
        return;
    auto img2 = QImage{mbr2->size(), QImage::Format_ARGB32_Premultiplied};
    auto inv = geometry::transformer(
        geometry::inverter(mbr, wnd.width(), wnd.height()),
        geometry::inverter(tf),
        geometry::forwarder(mbr1, img1.width(), img1.height()));
    auto ys = std::views::iota(0, img2.height());
    std::for_each(std::execution::par, ys.begin(), ys.end(), [&](int y) {
        int dx = mbr2->left(), dy = mbr2->top();
        auto rgb = reinterpret_cast<QRgb*>(img2.scanLine(y));
        for (int x{}; x < img2.width(); ++x)
            rgb[x] = inv(geometry::geographic::point(x + dx, y + dy))
                         .and_then([&](auto&& p) {
                             auto q = QPoint(p.x(), p.y());
                             return img1.valid(q) ? std::optional{img1.pixel(q)}
                                                  : std::nullopt;
                         })
                         .value_or(Qt::transparent);
    });
    paint.drawImage(mbr2->topLeft(), img2);
}

}  // namespace boat::gui::qt

#endif  // BOAT_GUI_QT_HPP
