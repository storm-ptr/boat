// Andrew Naplavkov

#ifndef BOAT_GUI_QT_HPP
#define BOAT_GUI_QT_HPP

#include <QImage>
#include <QPainter>
#include <QPainterPath>
#include <boat/gui/detail/geometry.hpp>
#include <boat/gui/feature.hpp>
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

void draw(shape const& feat,
          QPainter& art,
          geometry::matrix const& affine,
          geometry::srs_spec auto const& srs)
{
    auto var = forward(epsg(feat.epsg), affine, srs)(variant(feat.wkb));
    if (!var)
        return;
    overloaded{
        [&](geometry::point auto& g) { art.drawPoint(adapt(g)); },
        [&](geometry::linestring auto& g) { art.drawPolyline(adapt(g)); },
        [&](geometry::polygon auto& g) { art.drawPath(adapt(g)); },
        [](this auto&& self, geometry::multi auto& g) {
            std::ranges::for_each(g, self);
        },
        [](this auto&& self, geometry::dynamic auto& g) {
            std::visit(self, g);
        }}(*var);
}

void draw(raster const& feat,
          QPainter& art,
          geometry::matrix const& affine,
          geometry::srs_spec auto const& srs)
{
    auto img1 = QImage{};
    if (!img1.loadFromData(reinterpret_cast<uchar const*>(feat.image.data()),
                           static_cast<int>(feat.image.size())))
        return;
    auto [fwd, inv] = bidirectional(feat.affine, epsg(feat.epsg), affine, srs);
    auto mbr2 =
        fwd(multi_point(img1.width(), img1.height()))
            .transform(geometry::minmax)
            .transform(adapt)
            .transform(&QRectF::toAlignedRect)
            .transform(std::bind_front(&QRect::intersected, art.window()));
    if (!mbr2 || mbr2->isEmpty())
        return;
    auto img2 = QImage{mbr2->size(), QImage::Format_ARGB32_Premultiplied};
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
    art.drawImage(mbr2->topLeft(), img2);
}

}  // namespace detail

void draw(feature const& feat,
          QPainter& art,
          geometry::matrix const& affine,
          geometry::srs_spec auto const& srs)
{
    std::visit([&](auto const& v) { detail::draw(v, art, affine, srs); }, feat);
}

}  // namespace boat::gui::qt

#endif  // BOAT_GUI_QT_HPP
