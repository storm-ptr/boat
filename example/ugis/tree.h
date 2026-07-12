// Andrew Naplavkov

#ifndef TREE_H
#define TREE_H

#include <QBrush>
#include <QDataStream>
#include <QPen>
#include <QString>
#include <boat/db/meta.hpp>
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

using node = std::variant<branch, leaf>;

struct tree {
    node data;
    tree* parent = nullptr;
    std::vector<std::unique_ptr<tree>> children;
};

bool read(QString const& path, tree& out);
bool write(QString const& path, tree const& in);
QString to_string(tree*);

#endif  // TREE_H
