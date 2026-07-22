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
    QAction* act_outline{};
    QAction* act_paste{};
    QAction* act_refresh{};
    QAction* act_unmount{};
    QAction* act_width{};
    auto opt = model_.get_leaf(idx);
    if (opt || model_.is_branch(idx))
        act_describe = menu.addAction("describe");
    if (opt) {
        if (!opt->layer.raster) {
            act_fill = menu.addAction("filling color");
            act_outline = menu.addAction("outline color");
            act_width = menu.addAction("outline width");
            act_drop = menu.addAction("drop layer");
        }
        menu.addSeparator();
    }
    if (opt) {
        if (!opt->layer.raster)
            act_copy = menu.addAction("copy");
        act_copy_as = menu.addAction("copy as");
    }
    if (model_.can_paste_to(idx))
        act_paste = menu.addAction("paste");
    if (act_copy || act_copy_as || act_paste)
        menu.addSeparator();
    if (model_.can_refresh(idx))
        act_refresh = menu.addAction("refresh source");
    auto act_mount = menu.addAction("mount source");
    if (model_.is_mounted(idx))
        act_unmount = menu.addAction("unmount source");
    menu.addSeparator();
    auto act_new = menu.addAction("new workspace");
    auto act_open = menu.addAction("open workspace");
    auto act_save = menu.addAction("save workspace");
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
        auto dlg = QFileDialog{this};
        dlg.setAcceptMode(QFileDialog::AcceptSave);
        dlg.setNameFilter(copy_as_filter(opt->layer.raster));
        dlg.setOption(QFileDialog::DontUseNativeDialog);
        if (dlg.exec() != QDialog::Accepted)
            return;
        auto selected = dlg.selectedNameFilter();
        auto path = dlg.selectedFiles().value(0);
        if (path.isEmpty())
            return;
        auto fmt = copy_as_format(opt->layer.raster, selected);
        if (fmt) {
            model_.copy_as(
                idx, ensure_ext(std::move(path), fmt->ext), fmt->driver);
            return;
        }
        auto ok = false;
        auto driver = QInputDialog::getText(  //
            this,
            "copy as",
            "driver",
            QLineEdit::Normal,
            {},
            &ok);
        if (!ok || driver.isEmpty())
            return;
        model_.copy_as(idx, std::move(path), driver);
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
    else if (act == act_describe)
        model_.describe(idx);
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
        auto path =
            QFileDialog::getOpenFileName(this, {}, {}, workspace_filter);
        if (path.isEmpty())
            return;
        if (!model_.open_workspace(path))
            QMessageBox::warning(this, {}, "open workspace failed");
        else
            workspace_path_ = path;
    }
    else if (act == act_save) {
        auto path = QFileDialog::getSaveFileName(
            this, {}, workspace_path_, workspace_filter);
        if (path.isEmpty())
            return;
        path = ensure_ext(std::move(path), ".ugis");
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
