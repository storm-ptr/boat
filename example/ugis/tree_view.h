// Andrew Naplavkov

#ifndef TREE_VIEW_H
#define TREE_VIEW_H

#include <QString>
#include <QTreeView>
#include "tree_model.h"

class QContextMenuEvent;

class tree_view : public QTreeView {
    Q_OBJECT

public:
    explicit tree_view(QWidget* parent = nullptr);
    tree_model& model() { return model_; }

signals:
    void locate(leaf);

protected:
    void contextMenuEvent(QContextMenuEvent*) override;

private:
    tree_model model_;
    QString workspace_path_;
};

#endif  // TREE_VIEW_H
