// Andrew Naplavkov

#include <QPainter>
#include <boat/gui/qt.hpp>
#include "catalog.h"
#include "geometry.h"
#include "map_view.h"

namespace geo = boat::geometry;
using point = geo::geographic::point;

namespace {

std::optional<point> any_lonlat(leaf const& lyr, std::stop_token tok)
{
    if (tok.stop_requested())
        return {};
    auto cat = make_catalog(lyr.address);
    if (lyr.layer.raster) {
        auto rast = cat->get_raster(lyr.layer);
        auto crs = geo::srs::epsg(rast.epsg);
        auto x = rast.xorig + (rast.width / 2.) * rast.xscale;
        auto y = rast.yorig + (rast.height / 2.) * rast.yscale;
        return geo::transform(geo::srs_inverse(geo::transformation(crs)))(
            point{x, y});
    }
    auto tbl = cat->get_table(lyr.layer.schema_name, lyr.layer.table_name);
    auto& col = lyr.layer.column_name;
    auto it =
        std::ranges::find(tbl.columns, col, &boat::db::column::column_name);
    if (it == tbl.columns.end())
        return {};
    auto crs = geo::srs::epsg(it->epsg);
    if (tok.stop_requested())
        return {};
    auto rs =
        cat->select(tbl, boat::db::page{.select_list = {col}, .limit = 1});
    if (rs.empty())
        return {};
    auto wkb = std::get_if<boat::blob>(&rs.value());
    if (!wkb)
        return {};
    auto var = geo::geographic::variant{};
    boat::blob_view{*wkb} >> var;
    auto p = boat::overloaded{
        [](geo::point auto&& v) -> std::optional<point> {
            return point{v.x(), v.y()};
        },
        [](this auto&& self, geo::curve auto&& v) -> std::optional<point> {
            return v.empty() ? std::nullopt : self(v.front());
        },
        [](this auto&& self, geo::polygon auto&& v) -> std::optional<point> {
            return self(v.outer());
        },
        [](this auto&& self, geo::multi auto&& v) -> std::optional<point> {
            return v.empty() ? std::nullopt : self(v.front());
        },
        [](this auto&& self, geo::dynamic auto&& v) -> std::optional<point> {
            return std::visit(self, v);
        },
    }(var);
    return p ? geo::transform(geo::srs_inverse(geo::transformation(crs)))(*p)
             : std::nullopt;
}

}  // namespace

void map_view::locate(leaf lyr)
{
    tasks_.request_stop();
    watch_task(tasks_.run([=, lyr = std::move(lyr)](auto tok) {
        try {
            QMetaObject::invokeMethod(
                this,
                [=, mid = any_lonlat(lyr, tok)] {
                    if (tok.stop_requested())
                        return;
                    if (mid)
                        map_mid_ = geo::wrap(*mid);
                    update();
                    redraw();
                },
                Qt::QueuedConnection);
        }
        catch (std::exception const& e) {
            qWarning() << "locate error:" << e.what();
        }
    }));
}

void map_view::redraw()
{
    auto w = width(), h = height();
    if (w <= 0 || h <= 0)
        return;
    auto mid = map_mid_;
    auto res = map_res_;
    tasks_.request_stop();
    watch_task(tasks_.run([=, lyrs = layers_](auto tok) {
        if (tok.stop_requested())
            return;
        auto cats = std::map<std::string, std::unique_ptr<boat::db::catalog>>{};
        auto crs = geo::ortho(mid);
        auto mat = affine(w, h, mid, res, crs);
        auto num_points = static_cast<size_t>(
            (w * h) / (boat::tile::size * boat::tile::size) + 1);
        auto pvd = boat::gui::provider{
            .cache = cache_,
            .grid = geo::geographic_interpolate(w, h, mat, crs, num_points)};
        auto img = QImage{w, h, QImage::Format_RGBA8888};
        img.fill(Qt::white);
        auto art = QPainter{&img};
        art.setRenderHint(QPainter::Antialiasing);
        art.setCompositionMode(QPainter::CompositionMode_Darken);
        auto drw = boat::gui::draw_variant(std::execution::seq, art, mat, crs);
        for (auto& l : lyrs)
            try {
                if (tok.stop_requested())
                    return;
                pvd.catalog = [&] -> boat::db::catalog& {
                    auto& cat = cats[l.address];
                    if (!cat)
                        cat = make_catalog(l.address);
                    return *cat;
                };
                pvd.layer = l.layer;
                pvd.key = l.cache;
                art.setPen(l.pen);
                art.setBrush(l.brush);
                for (auto var : pvd.variants()) {
                    if (tok.stop_requested())
                        return;
                    std::visit(drw, var);
                }
            }
            catch (std::exception const& e) {
                qWarning() << "draw error:" << e.what();
            }
        QMetaObject::invokeMethod(
            this,
            [=, img = std::move(img)] mutable {
                if (tok.stop_requested())
                    return;
                img_ = std::move(img);
                img_mid_ = mid;
                img_res_ = res;
                update();
            },
            Qt::QueuedConnection);
    }));
}
