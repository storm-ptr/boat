// Andrew Naplavkov

#include <QDebug>
#include <boat/db/io.hpp>
#include <boat/gui/caches/cache.hpp>
#include <boat/gui/detail/geometry.hpp>
#include <filesystem>
#include "catalog.h"
#include "copy_layer.h"
#include "geometry.h"
#include "tree_model.h"

void tree_model::copy_layer_as(  //
    QModelIndex const& idx,
    QString const& path,
    QString const& driver)
{
    auto l = to_leaf(idx);
    if (!l)
        return;
    clipboard_.reset();
    auto lyr = *l;
    auto adr = path.toStdString();
    auto drv = driver.toStdString();
    tasks_.run([=](auto tok) {
        try {
            if (lyr.layer.raster)
                copy_raster(lyr, adr.data(), drv.data(), tok);
            else
                copy_vector(  //
                    lyr,
                    adr.data(),
                    drv.data(),
                    lyr.layer.table_name.data(),
                    tok);
            make_catalog(adr);
            QMetaObject::invokeMethod(
                this,
                [=] {
                    if (tok.stop_requested())
                        return;
                    mount(boat::db::source{
                        .source_name =
                            std::filesystem::path(adr).filename().string(),
                        .address = adr,
                    });
                    qInfo() << "copy_layer_as completed";
                },
                Qt::QueuedConnection);
        }
        catch (std::exception const& e) {
            qWarning() << e.what();
        }
    });
}

void tree_model::describe(QModelIndex const& idx)
{
    if (auto b = to_branch(idx); b && to_tree(idx) != root_.get()) {
        qInfo().noquote().nospace()
            << "\n"
            << QString::fromStdString(b->source.address);
        return;
    }
    auto l = to_leaf(idx);
    if (!l)
        return;
    tasks_.run([lyr = *l](auto tok) {
        try {
            if (tok.stop_requested())
                return;
            auto cat = make_catalog(lyr.address);
            if (lyr.layer.raster)
                qInfo().noquote() << boat::concat(cat->get_raster(lyr.layer));
            else
                qInfo().noquote() << boat::concat(cat->get_table(
                    lyr.layer.schema_name, lyr.layer.table_name));
        }
        catch (std::exception const& e) {
            qWarning() << e.what();
        }
    });
}

void tree_model::drop(QModelIndex const& idx)
{
    auto l = to_leaf(idx);
    if (!l || l->layer.raster)
        return;
    auto per = QPersistentModelIndex(idx);
    auto adr = l->address;
    auto scm = l->layer.schema_name;
    auto tbl = l->layer.table_name;
    tasks_.run([=](auto tok) {
        try {
            if (tok.stop_requested())
                return;
            make_catalog(adr)->drop(scm, tbl);
            qInfo() << "dropped"
                    << boat::concat(scm, scm.empty() ? "" : ".", tbl);
            QMetaObject::invokeMethod(
                this, [=] { on_dropped(per); }, Qt::QueuedConnection);
        }
        catch (std::exception const& e) {
            qWarning() << e.what();
        }
    });
}

void tree_model::fetchMore(QModelIndex const& idx)
{
    auto per = QPersistentModelIndex(idx);
    auto b = to_branch(per);
    if (!b || b->state != branch_state::blank)
        return;
    b->state = branch_state::fetching;
    tasks_.run([this, per, src = b->source](auto tok) {
        try {
            auto children = std::vector<std::unique_ptr<tree>>{};
            if (!tok.stop_requested()) {
                auto cat = make_catalog(src.address);
                for (auto& item : cat->sources())
                    children.push_back(std::make_unique<tree>(branch{item}));
                for (auto& item : cat->layers())
                    children.push_back(std::make_unique<tree>(
                        leaf{.address = src.address,
                             .layer = item,
                             .cache = boat::gui::caches::next_key()}));
                qInfo() << "fetched" << children.size() << "items from"
                        << src.source_name;
            }
            QMetaObject::invokeMethod(
                this,
                [this, tok, per, children = std::move(children)] mutable {
                    if (tok.stop_requested())
                        on_fetch_canceled(per);
                    else
                        on_fetched(per, std::move(children));
                },
                Qt::QueuedConnection);
        }
        catch (std::exception const& e) {
            qWarning() << e.what();
            QMetaObject::invokeMethod(
                this, [=] { on_fetch_canceled(per); }, Qt::QueuedConnection);
        }
    });
}

void tree_model::paste_layer(QModelIndex const& idx, QString const& name)
{
    if (!can_paste_to(idx))
        return;
    auto per = QPersistentModelIndex(idx);
    auto src = *clipboard_;
    auto b = to_branch(idx);
    tasks_.run([=, adr = b->source.address, nm = name.toStdString()](auto tok) {
        try {
            auto dst = copy_vector(src, adr.data(), nullptr, nm.data(), tok);
            QMetaObject::invokeMethod(
                this,
                [=] {
                    if (tok.stop_requested())
                        return;
                    on_pasted(per, dst);
                    qInfo() << "paste_layer completed";
                },
                Qt::QueuedConnection);
        }
        catch (std::exception const& e) {
            qWarning() << e.what();
        }
    });
}

void tree_model::sample(QModelIndex const& idx, viewport const& vp)
{
    auto l = to_leaf(idx);
    if (!l || l->layer.raster)
        return;
    tasks_.run([lyr = *l, vp](auto tok) {
        try {
            if (tok.stop_requested())
                return;
            auto cat = make_catalog(lyr.address);
            auto tbl =
                cat->get_table(lyr.layer.schema_name, lyr.layer.table_name);
            auto& col = lyr.layer.column_name;
            auto it = std::ranges::find(
                tbl.columns, col, &boat::db::column::column_name);
            if (it == tbl.columns.end())
                return;
            auto w = vp.width, h = vp.height;
            if (w <= 0 || h <= 0)
                return;
            auto ortho = boat::geometry::ortho(vp.mid_point);
            auto mat = affine(w, h, vp.mid_point, vp.resolution, ortho);
            auto num_points = static_cast<size_t>(
                (w * h) / (boat::tile::size * boat::tile::size) + 1);
            auto grid = boat::geometry::geographic_interpolate(
                w, h, mat, ortho, num_points);
            auto crs = boat::geometry::srs::epsg(it->epsg);
            for (auto& box : boat::gui::boxes(grid, crs)) {
                if (tok.stop_requested())
                    return;
                auto a = box.min_corner(), b = box.max_corner();
                auto rs = cat->select(
                    tbl,
                    boat::db::bbox{{}, col, a.x(), a.y(), b.x(), b.y(), 10});
                if (!rs.empty()) {
                    qInfo().noquote() << boat::concat(rs);
                    return;
                }
            }
            qInfo() << "sample: no data found";
        }
        catch (std::exception const& e) {
            qWarning() << e.what();
        }
    });
}
