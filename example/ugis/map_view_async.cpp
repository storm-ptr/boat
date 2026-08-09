// Andrew Naplavkov

#include <QPainter>
#include <boat/gui/qt.hpp>
#include "catalog.h"
#include "geometry.h"
#include "map_view.h"

namespace geo = boat::geometry;

void map_view::redraw()
{
    tasks_.request_stop();
    auto w = width(), h = height();
    if (w <= 0 || h <= 0)
        return;
    auto mid = map_mid_;
    auto scale = map_scale_;
    tasks_.run([=, lyrs = layers_](auto tok) {
        auto catalogs =
            std::map<std::string, std::unique_ptr<boat::db::catalog>>{};
        auto crs = geo::ortho(mid);
        auto mat = affine(w, h, mid, scale, crs);
        auto num_points = static_cast<size_t>(
            (w * h) / (boat::tile::size * boat::tile::size) + 4);
        auto grid = geo::geographic_interpolate(w, h, mat, crs, num_points);
        auto img = QImage{w, h, QImage::Format_RGBA8888};
        img.fill(Qt::white);
        auto art = QPainter{&img};
        art.setRenderHint(QPainter::Antialiasing);
        art.setCompositionMode(QPainter::CompositionMode_Darken);
        auto drw = boat::gui::draw_variant(std::execution::seq, art, mat, crs);
        for (auto& l : lyrs) {
            if (tok.stop_requested())
                return;
            try {
                if (!l.layer.raster) {
                    art.setPen(l.pen);
                    art.setBrush(l.brush);
                }
                auto pvd = boat::gui::provider{
                    .catalog = [&] -> boat::db::catalog& {
                        auto& cat = catalogs[l.address];
                        if (!cat)
                            cat = make_catalog(l.address);
                        return *cat;
                    },
                    .layer = l.layer,
                    .cache = cache_,
                    .key = l.cache,
                    .grid = grid,
                };
                for (auto var : pvd.variants()) {
                    if (tok.stop_requested())
                        return;
                    std::visit(drw, var);
                }
            }
            catch (std::exception const& e) {
                qWarning() << "draw error:" << e.what();
            }
        }
        QMetaObject::invokeMethod(
            this,
            [=, img = std::move(img)] mutable {
                if (tok.stop_requested())
                    return;
                img_ = std::move(img);
                img_mid_ = mid;
                img_scale_ = scale;
                update();
            },
            Qt::QueuedConnection);
    });
}
