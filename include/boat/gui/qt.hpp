// Andrew Naplavkov

#ifndef BOAT_GUI_QT_HPP
#define BOAT_GUI_QT_HPP

#include <QImage>
#include <QPainter>
#include <QPainterPath>
#include <boat/geometry/transform.hpp>
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
    auto fwd =
        geometry::transform(geometry::forward(tf),
                            geometry::forward(mbr, wnd.width(), wnd.height()));
    if (feat.raster.empty()) {
        if (auto pj = fwd(feat.shape))
            detail::draw(*pj, paint);
        return;
    }
    auto in_mbr = geometry::envelope(feat.shape);
    auto out_mbr = fwd(in_mbr)
                       .transform(detail::adapt)
                       .transform(&QRectF::toAlignedRect)
                       .transform(std::bind_front(&QRect::intersected, wnd));
    if (!out_mbr || out_mbr->isEmpty())
        return;
    auto in = QImage{};
    if (!in.loadFromData(reinterpret_cast<uchar const*>(feat.raster.data()),
                         static_cast<int>(feat.raster.size())))
        return;
    auto out = QImage{out_mbr->size(), QImage::Format_ARGB32_Premultiplied};
    auto inv =
        geometry::transform(geometry::inverse(mbr, wnd.width(), wnd.height()),
                            geometry::inverse(tf),
                            geometry::forward(in_mbr, in.width(), in.height()));
    auto ys = std::views::iota(0, out.height());
    std::for_each(std::execution::par, ys.begin(), ys.end(), [&](int y) {
        int dx = out_mbr->left(), dy = out_mbr->top();
        auto rgb = reinterpret_cast<QRgb*>(out.scanLine(y));
        for (int x{}; x < out.width(); ++x)
            rgb[x] = inv(geometry::geographic::point(x + dx, y + dy))
                         .and_then([&](auto&& p) {
                             auto q = QPoint(p.x(), p.y());
                             return in.valid(q) ? std::optional{in.pixel(q)}
                                                : std::nullopt;
                         })
                         .value_or(Qt::transparent);
    });
    paint.drawImage(out_mbr->topLeft(), out);
}

}  // namespace boat::gui::qt

#endif  // BOAT_GUI_QT_HPP
