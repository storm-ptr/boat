// Andrew Naplavkov

#include <QDebug>
#include <boat/db/io.hpp>
#include <boat/gui/describe.hpp>
#include <boat/gui/qt.hpp>
#include "db.h"
#include "feature.h"
#include "tree.h"
#include "viewport.h"

namespace db = boat::db;
namespace geo = boat::geometry;
using box = geo::geographic::box;

namespace {

auto hit_test(QRectF area, leaf const& lyr, auto const& fwd)
{
    auto empty = QImage{area.size().toSize(), QImage::Format_Alpha8};
    empty.fill(0);
    return [&, area, empty](auto const& ll) {
        auto xy = fwd(ll);
        if (!xy)
            return false;
        auto mask = empty;
        auto art = QPainter{&mask};
        art.setRenderHint(QPainter::Antialiasing);
        art.translate(-area.topLeft());
        art.setPen(lyr.pen);
        art.setBrush(lyr.brush);
        boat::gui::draw_geometry(art)(*xy);
        art.end();
        return mask != empty;
    };
}

auto footprint(QRectF area, double width)
{
    auto pad = std::max<>(1., width);
    area.adjust(-pad, -pad, pad, pad);
    return geo::box_interpolate(
        box{{area.left(), area.top()}, {area.right(), area.bottom()}}, 2, 0);
}

std::generator<feature> matching_rows(
    db::rowset rs,
    std::string const& name,
    ptrdiff_t layer,
    auto const& inv,
    auto const& match,
    auto& done,
    std::stop_token tok)
{
    auto idx = std::ranges::find(rs.columns, name) - rs.columns.begin();
    if (idx == std::ssize(rs.columns))
        co_return;
    auto vals = db::rowset{std::move(rs.columns), {{}}};
    for (auto& row : rs.rows) {
        if (tok.stop_requested())
            co_return;
        if (idx >= std::ssize(row) ||
            !std::holds_alternative<boat::blob>(row[idx]) ||
            std::ranges::contains(done, row))
            continue;
        auto geom = inv(db::get<geo::geographic::variant>(row[idx]));
        if (!geom || !match(*geom))
            continue;
        vals.rows.front() = std::move(row);
        auto text = QString::fromStdString(boat::concat(vals)).trimmed();
        co_yield {std::move(*geom), layer, std::move(text)};
        done.push_back(std::move(vals.rows.front()));
    }
}

auto read_layer(boat::gui::caches::cache* cache, int limit, std::stop_token tok)
{
    return [=, cats = catalog_map{}](
               ptrdiff_t i,
               leaf const& lyr,
               auto ll,
               auto match) mutable -> std::generator<feature> {
        auto cat = [&]() -> auto& { return get_catalog(cats, lyr.address); };
        auto tbl = boat::gui::get_table(cache, lyr.cache_key, lyr.layer, cat);
        auto& col = lyr.layer.column_name;
        auto& fld = db::find(tbl.columns, col);
        auto tf =
            geo::transformation(geo::make_srs_variant(fld.epsg, fld.proj4));
        auto fwd = geo::transform(geo::forward(tf));
        auto inv = geo::transform(geo::backward(tf));
        auto done = decltype(db::rowset::rows){};
        auto rq = db::bbox{.layer_column = col, .limit = limit};
        for (auto& c : tbl.columns)
            if (!c.kind.empty())
                rq.select_list.push_back(c.column_name);
        auto boxes = geo::split_at_antimeridian(return_envelope<box>(ll));
        for (auto& part : boxes) {
            if (boxes.size() > 1)
                ll = geo::box_interpolate(part, 2, 0);
            auto xy = fwd(std::move(ll));
            if (tok.stop_requested())
                co_return;
            if (!xy)
                continue;
            rq.bounds = geo::cartesian_envelope(*xy);
            auto rs = cat().select(tbl, rq, tok);
            co_yield std::ranges::elements_of(
                matching_rows(std::move(rs), col, i, inv, match, done, tok));
        }
    };
}

}  // namespace

std::vector<feature> identify_features(
    std::vector<leaf> const& layers,
    QPointF cursor,
    viewport const& vp,
    boat::gui::caches::cache* cache,
    int query_limit,
    int feature_limit,
    std::stop_token tok)
{
    if (tok.stop_requested() || query_limit <= 0 || feature_limit <= 0)
        return {};
    auto const& [v, ext] = vp;
    auto tf = geo::transformation(geo::ortho(v.center));
    auto mat = geo::affine(ext, v.center, v.resolution, tf);
    auto fwd = geo::transform(geo::forward(tf), geo::backward(mat));
    auto inv = geo::transform(geo::forward(mat), geo::backward(tf));
    auto read = read_layer(cache, query_limit, tok);
    auto ret = std::vector<feature>{};
    auto pixel = QRectF{cursor, QSizeF{1, 1}};
    for (auto area : {pixel, pixel.adjusted(-1, -1, 1, 1)}) {
        area &= QRect{0, 0, ext.x(), ext.y()};
        for (auto [i, lyr] : std::views::enumerate(layers))
            try {
                if (tok.stop_requested())
                    return ret;
                if (lyr.layer.raster)
                    continue;
                auto ll = inv(footprint(area, lyr.pen.widthF()));
                if (!ll)
                    continue;
                for (auto&& hit :
                     read(i, lyr, std::move(*ll), hit_test(area, lyr, fwd))) {
                    ret.push_back(std::move(hit));
                    if (std::ssize(ret) == feature_limit)
                        return ret;
                }
            }
            catch (std::exception const& e) {
                qWarning() << "identify error:" << e.what();
            }
        if (!ret.empty())
            break;
    }
    return ret;
}
