// Andrew Naplavkov

#include <QPainter>
#include <boat/gui/qt.hpp>
#include <boat/gui/renderables.hpp>
#include <boat/gui/utility.hpp>
#include "db.h"
#include "map_view.h"
#include "tree.h"

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
        auto crs = boat::gui::to_srs_variant(rast);
        return geo::transform(
            geo::mat_forward(boat::gui::affine(rast)),
            geo::srs_inverse(geo::transformation(crs)))(
            point{rast.width * .5, rast.height * .5});
    }
    auto tbl = cat->get_table(lyr.layer.schema_name, lyr.layer.table_name);
    auto& col = lyr.layer.column_name;
    auto it =
        std::ranges::find(tbl.columns, col, &boat::db::column::column_name);
    if (it == tbl.columns.end())
        return {};
    auto crs = boat::gui::to_srs_variant(*it);
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
    auto p = [](this auto&& self, auto const& v) -> std::optional<point> {
        if constexpr (geo::point<decltype(v)>)
            return point{v.x(), v.y()};
        else if constexpr (geo::polygon<decltype(v)>)
            return self(v.outer());
        else if constexpr (geo::dynamic<decltype(v)>)
            return std::visit(self, v);
        else
            return v.empty() ? std::nullopt : self(v.front());
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
            post(this, [=, mid = any_lonlat(lyr, tok)] {
                if (tok.stop_requested())
                    return;
                if (mid)
                    map_ = bounded({*mid, map_.resolution}).value_or(map_);
                update();
                redraw();
            });
        }
        catch (std::exception const& e) {
            qWarning() << "locate error:" << e.what();
        }
    }));
}

void map_view::redraw()
{
    auto [view, w, h] = this->view();
    if (w <= 0 || h <= 0)
        return;
    tasks_.request_stop();
    watch_task(tasks_.run([=, lyrs = layers_](auto tok) {
        if (tok.stop_requested())
            return;
        auto cats = std::map<std::string, std::unique_ptr<boat::db::catalog>>{};
        auto crs = geo::ortho(view.center);
        auto mat = affine(w, h, view, crs);
        auto params = boat::gui::render_params{
            .cache = cache_,
            .filter = geo::geographic_interpolate(
                w, h, mat, crs, view.resolution * boat::tile::size),
            .stop = tok};
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
                params.catalog = [&] -> boat::db::catalog& {
                    auto& cat = cats[l.address];
                    if (!cat)
                        cat = make_catalog(l.address);
                    return *cat;
                };
                params.layer = l.layer;
                params.cache_key = l.cache;
                art.setPen(l.pen);
                art.setBrush(l.brush);
                for (auto var : renderables(params)) {
                    if (tok.stop_requested())
                        return;
                    auto vec = std::get_if<boat::gui::vector>(&var);
                    art.setOpacity(vec && vec->partial ? .25 : 1.);
                    std::visit(drw, var);
                }
            }
            catch (std::exception const& e) {
                qWarning() << "draw error:" << e.what();
            }
        post(this, [=, img = std::move(img)] mutable {
            if (tok.stop_requested())
                return;
            img_ = std::move(img);
            img_view_ = view;
            update();
        });
    }));
}
