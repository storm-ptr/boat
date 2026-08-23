// Andrew Naplavkov

#ifndef TREE_VIEW_H
#define TREE_VIEW_H

#include <QString>
#include <QTreeView>
#include <functional>
#include "tree_model.h"

class QContextMenuEvent;
class map_view;

class tree_view : public QTreeView {
public:
    explicit tree_view(QWidget* parent = nullptr);
    tree_model& model() { return model_; }
    void set_map_view(map_view* map) { map_ = map; }
    void set_sql_handler(std::function<void(boat::db::source const&)>);

protected:
    void contextMenuEvent(QContextMenuEvent*) override;

private:
    map_view* map_{};
    std::function<void(boat::db::source const&)> sql_handler_;
    tree_model model_;
    QString workspace_path_;
};

#endif  // TREE_VIEW_H
