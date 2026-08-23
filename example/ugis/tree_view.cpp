// Andrew Naplavkov

#include <QClipboard>
#include <QColorDialog>
#include <QContextMenuEvent>
#include <QFileDialog>
#include <QGuiApplication>
#include <QHeaderView>
#include <QInputDialog>
#include <QLineEdit>
#include <QMenu>
#include <QMessageBox>
#include <QPainter>
#include <boat/detail/uri.hpp>
#include "formats.h"
#include "select_source_dialog.h"
#include "tree_view.h"

namespace {

template <class Get, class Set>
void pick_color(  //
    tree_view* self,
    tree_model& model,
    QModelIndex const& idx,
    char const* title,
    Get get,
    Set set)
{
    auto opt = model.get_leaf(idx);
    if (!opt || opt->layer.raster)
        return;
    auto color = QColorDialog::getColor(get(*opt), self, title);
    if (!color.isValid())
        return;
    model.mutate_leaf(idx, [&](leaf& l) { set(l, color); });
}

}  // namespace

tree_view::tree_view(QWidget* parent) : QTreeView(parent), model_(this)
{
    setModel(&model_);
    setRootIsDecorated(true);
    header()->hide();
}

void tree_view::contextMenuEvent(QContextMenuEvent* event)
{
    auto idx = indexAt(event->pos());
    auto menu = QMenu{this};
    auto opt = model_.get_leaf(idx);
    auto is_vector = opt && !opt->layer.raster;
    auto source = model_.get_source(idx);
    auto add = [&menu, this](bool on, char const* text, auto fn) {
        if (on)
            menu.addAction(text, this, std::move(fn));
        return on;
    };

    // other
    auto populated = false;
    populated |= add(opt || model_.is_branch(idx),
                     "describe in log",
                     [this, idx] { model_.describe(idx); });
    populated |= add(is_vector, "drop layer", [this, idx] {
        if (QMessageBox::question(  //
                this,
                {},
                "drop layer?",
                QMessageBox::Yes | QMessageBox::No,
                QMessageBox::No) == QMessageBox::Yes)
            model_.drop(idx);
    });
    populated |= add(bool(opt), "locate on map", [this, opt] {
        if (map_)
            map_->locate(*opt);
    });
    populated |= add(model_.can_refresh(idx), "refresh source", [this, idx] {
        model_.refresh(idx);
    });
    populated |= add(is_vector && map_, "sample in log", [this, idx] {
        if (map_)
            model_.sample(idx, map_->view());
    });
    populated |=
        add(source && sql_handler_ && !boat::is_http_url(source->address),
            "sql",
            [this, source] { sql_handler_(*source); });
    if (populated)
        menu.addSeparator();

    // color/width
    if (is_vector) {
        add(true, "filling color", [this, idx] {
            pick_color(
                this,
                model_,
                idx,
                "filling color",
                [](leaf const& l) { return l.brush.color(); },
                [](leaf& l, QColor c) { l.brush.setColor(c); });
        });
        add(true, "outline color", [this, idx] {
            pick_color(
                this,
                model_,
                idx,
                "outline color",
                [](leaf const& l) { return l.pen.color(); },
                [](leaf& l, QColor c) { l.pen.setColor(c); });
        });
        add(true, "outline width", [this, idx, opt] {
            auto ok = false;
            auto width = QInputDialog::getInt(
                this, "outline width", {}, opt->pen.width(), 0, 100, 1, &ok);
            if (ok)
                model_.mutate_leaf(idx,
                                   [&](leaf& l) { l.pen.setWidth(width); });
        });
        menu.addSeparator();
    }

    // copy/paste
    populated = false;
    populated |=
        add(is_vector, "copy layer", [this, idx] { model_.copy_layer(idx); });
    populated |= add(bool(opt), "copy layer as", [this, idx, opt] {
        auto selected = QString{};
        auto path = QFileDialog::getSaveFileName(  //
            this,
            {},
            {},
            copy_as_filter(opt->layer.raster),
            &selected,
            QFileDialog::DontUseNativeDialog);
        if (path.isEmpty())
            return;
        auto fmt = copy_as_format(opt->layer.raster, selected);
        if (fmt)
            model_.copy_layer_as(
                idx,
                ensure_extension(std::move(path), fmt->extension),
                fmt->driver);
    });
    populated |= add(is_vector, "copy name", [opt] {
        auto schema = QString::fromStdString(opt->layer.schema_name);
        auto table = QString::fromStdString(opt->layer.table_name);
        QGuiApplication::clipboard()->setText(
            schema.isEmpty() ? table : schema + "." + table);
    });
    populated |= add(model_.can_paste_to(idx), "paste layer", [this, idx] {
        auto ok = false;
        auto name = QInputDialog::getText(  //
            this,
            "paste layer",
            "table name",
            QLineEdit::Normal,
            model_.clipboard_name(),
            &ok);
        if (ok && !name.isEmpty())
            model_.paste_layer(idx, name);
    });
    if (populated)
        menu.addSeparator();

    // mount/unmount
    add(true, "mount source", [this] {
        auto dlg = select_source_dialog{this};
        if (dlg.exec() == QDialog::Accepted)
            model_.mount({
                .source_name = dlg.name().toStdString(),
                .address = dlg.address().toStdString(),
            });
    });
    add(model_.is_mounted(idx), "unmount source", [this, idx] {
        model_.unmount(idx);
    });
    menu.addSeparator();

    // workspace
    add(true, "new workspace", [this] {
        model_.new_workspace();
        workspace_path_.clear();
    });
    add(true, "open workspace", [this] {
        auto path = QFileDialog::getOpenFileName(  //
            this,
            {},
            {},
            workspace_filter,
            nullptr,
            QFileDialog::DontUseNativeDialog);
        if (path.isEmpty())
            return;
        if (!model_.open_workspace(path))
            QMessageBox::warning(this, {}, "open workspace failed");
        else
            workspace_path_ = path;
    });
    add(true, "save workspace", [this] {
        auto path = QFileDialog::getSaveFileName(  //
            this,
            {},
            workspace_path_,
            workspace_filter,
            nullptr,
            QFileDialog::DontUseNativeDialog);
        if (path.isEmpty())
            return;
        path = ensure_extension(std::move(path), ".ugis");
        if (!model_.save_workspace(path))
            QMessageBox::warning(this, {}, "save workspace failed");
        else
            workspace_path_ = path;
    });

    // cancel
    if (model_.busy()) {
        menu.addSeparator();
        add(true, "cancel tasks", [this] {
            if (QMessageBox::question(  //
                    this,
                    {},
                    "cancel tasks?",
                    QMessageBox::Yes | QMessageBox::No,
                    QMessageBox::No) == QMessageBox::Yes)
                model_.request_stop();
        });
    }

    menu.exec(event->globalPos());
}

void tree_view::paintEvent(QPaintEvent* event)
{
    QTreeView::paintEvent(event);
    if (model_.rowCount())
        return;
    auto art = QPainter{viewport()};
    art.setPen(palette().color(QPalette::PlaceholderText));
    art.drawText(viewport()->rect(),
                 Qt::AlignCenter | Qt::TextSingleLine,
                 "right-click to mount a source or open a workspace");
}

void tree_view::set_sql_handler(std::function<void(boat::db::source const&)> fn)
{
    sql_handler_ = std::move(fn);
}
