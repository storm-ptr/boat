// Andrew Naplavkov

#include <QDragEnterEvent>
#include <QDropEvent>
#include <QHeaderView>
#include <QMessageBox>
#include <QMimeData>
#include <QPainter>
#include <QPushButton>
#include <QUrl>
#include <algorithm>
#include "tree_view.h"

tree_view::tree_view(QWidget* parent) : QTreeView(parent), model_(this)
{
    setModel(&model_);
    setAcceptDrops(true);
    setRootIsDecorated(true);
    header()->hide();
}

void tree_view::replace_workspace(QString const& path)
{
    if (model_.rowCount()) {
        auto box = QMessageBox{
            QMessageBox::Question,
            {},
            "replace workspace?",
            QMessageBox::Cancel,
            this,
        };
        auto replace = box.addButton("replace", QMessageBox::AcceptRole);
        box.setDefaultButton(QMessageBox::Cancel);
        box.exec();
        if (box.clickedButton() != replace)
            return;
    }
    if (path.isEmpty())
        model_.new_workspace();
    else if (!model_.open_workspace(path)) {
        QMessageBox::warning(this, {}, "open workspace failed");
        return;
    }
    workspace_path_ = path;
}

void tree_view::dragEnterEvent(QDragEnterEvent* event)
{
    dragMoveEvent(event);
}

void tree_view::dragMoveEvent(QDragMoveEvent* event)
{
    if (!(event->possibleActions() & Qt::CopyAction) ||
        !std::ranges::any_of(event->mimeData()->urls(), &QUrl::isLocalFile)) {
        event->ignore();
        return;
    }
    event->setDropAction(Qt::CopyAction);
    event->accept();
}

void tree_view::dropEvent(QDropEvent* event)
{
    auto urls = event->mimeData()->urls();
    if (urls.size() == 1 && urls.front().isLocalFile() &&
        urls.front().fileName().endsWith(".ugis", Qt::CaseInsensitive)) {
        replace_workspace(urls.front().toLocalFile());
        event->setDropAction(Qt::CopyAction);
        event->accept();
        return;
    }
    for (auto& url : urls)
        if (url.isLocalFile())
            model_.mount({.source_name = url.fileName().toStdString(),
                          .address = url.toLocalFile().toStdString()});
    event->setDropAction(Qt::CopyAction);
    event->accept();
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
