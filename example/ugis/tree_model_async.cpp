// Andrew Naplavkov

#include <QDebug>
#include <boat/catalogs.hpp>
#include <boat/db/io.hpp>
#include <boat/gui/caches/cache.hpp>
#include <filesystem>
#include "copy_layer.h"
#include "tree_model.h"

void tree_model::copy_as(  //
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
                },
                Qt::QueuedConnection);
        }
        catch (std::exception const& e) {
            qWarning() << e.what();
        }
    });
}

void tree_model::paste(QModelIndex const& idx, QString const& name)
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
                    qInfo() << "paste completed";
                },
                Qt::QueuedConnection);
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
            boat::make_catalog(adr)->drop(scm, tbl);
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
                auto cat = boat::make_catalog(src.address);
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
            auto cat = boat::make_catalog(lyr.address);
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
