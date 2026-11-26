// Andrew Naplavkov

#include <boat/gui/fetch.hpp>
#include <boat/gui/qt.hpp>
#include "db.h"
#include "feature.h"
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
        auto crs = geo::make_srs_variant(rast.epsg, rast.proj4);
        return geo::transform(
            geo::forward(affine(rast)),
            geo::backward(geo::transformation(crs)))(
            point{rast.extent.x() * .5, rast.extent.y() * .5});
    }
    auto tbl = cat->get_table(lyr.layer.schema_name, lyr.layer.table_name);
    auto& col = lyr.layer.column_name;
    auto it =
        std::ranges::find(tbl.columns, col, &boat::db::column::column_name);
    if (it == tbl.columns.end() || tok.stop_requested())
        return {};
    auto crs = geo::make_srs_variant(it->epsg, it->proj4);
    auto rs =
        cat->select(tbl, boat::db::page{.select_list = {col}, .limit = 1}, tok);
    if (rs.empty() || !std::holds_alternative<boat::blob>(rs.value()))
        return {};
    auto var = boat::db::get<geo::geographic::variant>(rs.value());
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
    return p ? geo::transform(geo::backward(geo::transformation(crs)))(*p)
             : std::nullopt;
}

}  // namespace

void map_view::identify(QPointF cursor, viewport vp)
{
    clear_identify();
    watch_task(query_tasks_.run([=, lyrs = layers_](auto tok) {
        auto hits = identify_features(
            lyrs, cursor, vp, cache_.get(), query_limit, feature_limit, tok);
        post(this, [this, tok, hits = std::move(hits)] mutable {
            if (!tok.stop_requested())
                show_hits(std::move(hits));
        });
    }));
}

void map_view::locate(leaf lyr)
{
    query_tasks_.request_stop();
    watch_task(query_tasks_.run([=, lyr = std::move(lyr)](auto tok) {
        try {
            post(this, [=, mid = any_lonlat(lyr, tok)] {
                if (tok.stop_requested() || !mid)
                    return;
                map_ = bounded({*mid, map_.resolution}).value_or(map_);
                schedule_paint();
            });
        }
        catch (std::exception const& e) {
            qWarning() << "locate error:" << e.what();
        }
    }));
}

void map_view::redraw()
{
    auto [view, ext] = this->view();
    if (ext.x() <= 0 || ext.y() <= 0)
        return;
    watch_task(render_tasks_.run([=, lyrs = layers_](auto tok) {
        if (tok.stop_requested())
            return;
        auto cats = catalog_map{};
        auto crs = geo::ortho(view.center);
        auto mat = geo::affine(ext, view.center, view.resolution, crs);
        auto query = boat::gui::layer_query{
            .cache = cache_,
            .footprint = geo::geographic_interpolate(
                ext, mat, crs, view.resolution * boat::tile::half_size),
            .limit = query_limit,
            .stop = tok};
        auto img = QImage{ext.x(), ext.y(), QImage::Format_RGBA8888};
        img.fill(Qt::white);
        auto art = QPainter{&img};
        art.setRenderHint(QPainter::Antialiasing);
        art.setCompositionMode(QPainter::CompositionMode_Darken);
        auto drw = boat::gui::draw_variant(std::execution::seq, art, mat, crs);
        for (auto& l : lyrs)
            try {
                if (tok.stop_requested())
                    return;
                query.cache_key = l.cache_key;
                query.layer = l.layer;
                query.catalog = [&]() -> auto& {
                    return get_catalog(cats, l.address);
                };
                art.setPen(l.pen);
                art.setBrush(l.brush);
                for (auto var : fetch(query)) {
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
        art.end();
        post(this, [=, img = std::move(img)] mutable {
            if (tok.stop_requested())
                return;
            img_ = std::move(img);
            img_view_ = view;
            update();
        });
    }));
}
