// Andrew Naplavkov

#ifndef TREE_VIEW_H
#define TREE_VIEW_H

#include <QString>
#include <QTreeView>
#include "tree_model.h"

class QContextMenuEvent;

class tree_view : public QTreeView {
public:
    explicit tree_view(QWidget* parent = nullptr);

protected:
    void contextMenuEvent(QContextMenuEvent*) override;

private:
    tree_model model_;
    QString workspace_path_;
};

#endif  // TREE_VIEW_H
