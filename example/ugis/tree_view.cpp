// Andrew Naplavkov

#include <QColorDialog>
#include <QContextMenuEvent>
#include <QFileDialog>
#include <QHeaderView>
#include <QInputDialog>
#include <QLineEdit>
#include <QMenu>
#include <QMessageBox>
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
    QAction* act_cancel{};
    QAction* act_copy{};
    QAction* act_copy_as{};
    QAction* act_describe{};
    QAction* act_drop{};
    QAction* act_fill{};
    QAction* act_locate{};
    QAction* act_mount{};
    QAction* act_new{};
    QAction* act_open{};
    QAction* act_outline{};
    QAction* act_paste{};
    QAction* act_refresh{};
    QAction* act_sample{};
    QAction* act_save{};
    QAction* act_unmount{};
    QAction* act_width{};
    auto opt = model_.get_leaf(idx);
    auto is_vector = opt && !opt->layer.raster;

    // other
    if (opt || model_.is_branch(idx))
        act_describe = menu.addAction("describe in log");
    if (is_vector)
        act_drop = menu.addAction("drop layer");
    if (opt)
        act_locate = menu.addAction("locate on map");
    if (model_.can_refresh(idx))
        act_refresh = menu.addAction("refresh source");
    if (is_vector && map_)
        act_sample = menu.addAction("sample in log");
    if (act_describe || act_drop || act_locate || act_refresh || act_sample)
        menu.addSeparator();

    // color/width
    if (is_vector) {
        act_fill = menu.addAction("filling color");
        act_outline = menu.addAction("outline color");
        act_width = menu.addAction("outline width");
        menu.addSeparator();
    }

    // copy/paste
    if (is_vector)
        act_copy = menu.addAction("copy");
    if (opt)
        act_copy_as = menu.addAction("copy as");
    if (model_.can_paste_to(idx))
        act_paste = menu.addAction("paste");
    if (act_copy || act_copy_as || act_paste)
        menu.addSeparator();

    // mount/unmount
    act_mount = menu.addAction("mount source");
    if (model_.is_mounted(idx))
        act_unmount = menu.addAction("unmount source");
    menu.addSeparator();

    // workspace
    act_new = menu.addAction("new workspace");
    act_open = menu.addAction("open workspace");
    act_save = menu.addAction("save workspace");

    // cancel
    if (model_.busy()) {
        menu.addSeparator();
        act_cancel = menu.addAction("cancel tasks");
    }

    auto act = menu.exec(event->globalPos());
    if (act == act_fill)
        pick_color(
            this,
            model_,
            idx,
            "filling color",
            [](leaf const& l) { return l.brush.color(); },
            [](leaf& l, QColor c) { l.brush.setColor(c); });
    else if (act == act_outline)
        pick_color(
            this,
            model_,
            idx,
            "outline color",
            [](leaf const& l) { return l.pen.color(); },
            [](leaf& l, QColor c) { l.pen.setColor(c); });
    else if (act == act_width) {
        if (!opt || opt->layer.raster)
            return;
        auto ok = false;
        auto width = QInputDialog::getInt(
            this, "outline width", {}, opt->pen.width(), 0, 100, 1, &ok);
        if (ok)
            model_.mutate_leaf(idx, [&](leaf& l) { l.pen.setWidth(width); });
    }
    else if (act == act_refresh)
        model_.refresh(idx);
    else if (act == act_copy) {
        if (!opt || opt->layer.raster)
            return;
        model_.copy(idx);
    }
    else if (act == act_copy_as) {
        if (!opt)
            return;
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
        if (!fmt)
            return;
        model_.copy_as(  //
            idx,
            ensure_extension(std::move(path), fmt->extension),
            fmt->driver);
    }
    else if (act == act_paste) {
        auto ok = false;
        auto name = QInputDialog::getText(  //
            this,
            "paste layer",
            "table name",
            QLineEdit::Normal,
            model_.clipboard_name(),
            &ok);
        if (!ok || name.isEmpty())
            return;
        model_.paste(idx, name);
    }
    else if (act == act_drop) {
        if (QMessageBox::question(  //
                this,
                {},
                "drop layer?",
                QMessageBox::Yes | QMessageBox::No,
                QMessageBox::No) == QMessageBox::Yes)
            model_.drop(idx);
    }
    else if (act == act_sample) {
        if (!opt || opt->layer.raster || !map_)
            return;
        model_.sample(idx, map_->view());
    }
    else if (act == act_describe)
        model_.describe(idx);
    else if (act == act_locate) {
        if (!opt || !map_)
            return;
        map_->locate(*opt);
    }
    else if (act == act_mount) {
        auto dlg = select_source_dialog{this};
        if (dlg.exec() != QDialog::Accepted)
            return;
        model_.mount({
            .source_name = dlg.name().toStdString(),
            .address = dlg.address().toStdString(),
        });
    }
    else if (act == act_unmount)
        model_.unmount(idx);
    else if (act == act_new) {
        model_.new_workspace();
        workspace_path_.clear();
    }
    else if (act == act_open) {
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
    }
    else if (act == act_save) {
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
    }
    else if (act == act_cancel) {
        if (QMessageBox::question(  //
                this,
                {},
                "cancel tasks?",
                QMessageBox::Yes | QMessageBox::No,
                QMessageBox::No) == QMessageBox::Yes)
            model_.request_stop();
    }
}
