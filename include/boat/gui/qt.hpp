// Andrew Naplavkov

#ifndef BOAT_GUI_QT_HPP
#define BOAT_GUI_QT_HPP

#include <QImage>
#include <QPainter>
#include <QPainterPath>
#include <boat/gui/detail/geometry.hpp>
#include <boat/gui/variant.hpp>
#include <execution>

namespace boat::gui {

struct qt {
    using image = QImage;

    static constexpr auto fit = overloaded{
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

    static auto drawGeometry(QPainter& out)
    {
        return overloaded{
            [&](geometry::point auto&& g) { out.drawPoint(fit(g)); },
            [&](geometry::linestring auto&& g) { out.drawPolyline(fit(g)); },
            [&](geometry::polygon auto&& g) { out.drawPath(fit(g)); },
            [](this auto&& self, geometry::multi auto&& g) -> void {
                std::ranges::for_each(g, self);
            },
            [](this auto&& self, geometry::dynamic auto&& g) -> void {
                std::visit(self, g);
            }};
    }

    static bool loadImage(blob_view in, QImage& out)
    {
        return out.loadFromData(reinterpret_cast<uchar const*>(in.data()),
                                static_cast<int>(in.size()));
    }

    static void drawImage(  //
        QImage const& in,
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
                .transform(fit)
                .transform(&QRectF::toAlignedRect)
                .transform(std::bind_front(&QRect::intersected, out.window()));
        if (!mbr || mbr->isEmpty())
            return;
        auto img = QImage{mbr->size(), QImage::Format_ARGB32_Premultiplied};
        auto ys = std::views::iota(0, img.height());
        std::for_each(std::execution::par, ys.begin(), ys.end(), [&](int y) {
            int dx = mbr->left(), dy = mbr->top();
            auto rgb = reinterpret_cast<QRgb*>(img.scanLine(y));
            for (int x{}; x < img.width(); ++x)
                rgb[x] = inv(geometry::geographic::point(x + dx, y + dy))
                             .and_then([&](auto&& p) {
                                 auto q = QPoint(p.x(), p.y());
                                 return in.valid(q) ? std::optional{in.pixel(q)}
                                                    : std::nullopt;
                             })
                             .value_or(Qt::transparent);
        });
        out.drawImage(mbr->topLeft(), img);
    }
};

template <>
struct traits<QPainter> {
    using type = qt;
};

}  // namespace boat::gui

#endif  // BOAT_GUI_QT_HPP
