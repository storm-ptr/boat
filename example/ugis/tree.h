// Andrew Naplavkov

#ifndef TREE_H
#define TREE_H

#include <QBrush>
#include <QPen>
#include <QString>
#include <boat/gui/scene.hpp>
#include <memory>
#include <variant>

enum class branch_state { blank, fetching, ready };

struct branch {
    boat::db::source source;
    branch_state state = {};
};

struct leaf {
    std::string address;
    boat::db::layer layer;
    QPen pen;
    QBrush brush;
    Qt::CheckState state = Qt::Unchecked;
    size_t cache = 0;
};

struct tree {
    std::variant<branch, leaf> data;
    tree* parent = nullptr;
    std::vector<std::unique_ptr<tree>> children;
};

boat::db::source named_source(boat::db::source, std::string fallback = "source");
std::unique_ptr<tree> from_scene(boat::gui::scene const&);
boat::gui::scene to_scene(tree const&);
boat::gui::scene read_scene(QString const& path);
bool write_scene(QString const& path, boat::gui::scene const&);
QString to_string(tree*);

#endif  // TREE_H
