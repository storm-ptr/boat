// Andrew Naplavkov

#include "tree_model.h"

tree_model::tree_model(QObject* parent)
    : QAbstractItemModel(parent)
    , root_(std::make_unique<tree>(branch{.state = branch_state::ready}))
{
}

tree* tree_model::to_tree(QModelIndex const& idx) const
{
    if (idx.isValid())
        return static_cast<tree*>(idx.internalPointer());
    return root_.get();
}

branch* tree_model::to_branch(QModelIndex const& idx) const
{
    return std::get_if<branch>(&to_tree(idx)->data);
}

leaf* tree_model::to_leaf(QModelIndex const& idx) const
{
    return std::get_if<leaf>(&to_tree(idx)->data);
}

std::optional<leaf> tree_model::get_leaf(QModelIndex const& idx) const
{
    if (auto l = to_leaf(idx))
        return *l;
    return {};
}

bool tree_model::is_branch(QModelIndex const& idx) const
{
    return to_branch(idx) && to_tree(idx) != root_.get();
}

bool tree_model::is_mounted(QModelIndex const& idx) const
{
    return idx.isValid() && to_tree(idx)->parent == root_.get();
}

bool tree_model::can_paste_to(QModelIndex const& idx) const
{
    return clipboard_ && !clipboard_->layer.raster && is_branch(idx);
}

QString tree_model::clipboard_name() const
{
    return clipboard_ ? QString::fromStdString(clipboard_->layer.table_name)
                      : QString{};
}

bool tree_model::can_refresh(QModelIndex const& idx) const
{
    auto b = to_branch(idx);
    return b && to_tree(idx) != root_.get() &&
           b->state != branch_state::fetching;
}

void tree_model::mount(boat::db::source const& val)
{
    auto& children = root_->children;
    int row = static_cast<int>(children.size());
    beginInsertRows({}, row, row);
    auto ch = std::make_unique<tree>(branch{val});
    ch->parent = root_.get();
    children.push_back(std::move(ch));
    endInsertRows();
}

void tree_model::unmount(QModelIndex const& idx)
{
    if (is_mounted(idx))
        removeRows(idx.row(), 1);
}

void tree_model::refresh(QModelIndex const& idx)
{
    if (!can_refresh(idx))
        return;
    to_branch(idx)->state = branch_state::blank;
    if (int count = static_cast<int>(to_tree(idx)->children.size()))
        removeRows(0, count, idx);
    else
        emit dataChanged(idx, idx);
}

void tree_model::new_workspace()
{
    beginResetModel();
    root_ = std::make_unique<tree>(branch{.state = branch_state::ready});
    endResetModel();
}

bool tree_model::open_workspace(QString const& path)
{
    auto root = std::make_unique<tree>(branch{.state = branch_state::ready});
    if (!read(path, *root))
        return false;
    beginResetModel();
    root_ = std::move(root);
    endResetModel();
    return true;
}

bool tree_model::save_workspace(QString const& path) const
{
    return write(path, *root_);
}

QVariant tree_model::data(QModelIndex const& idx, int role) const
{
    switch (role) {
        case Qt::DisplayRole:
            return to_string(to_tree(idx));
        case Qt::CheckStateRole:
            if (auto l = to_leaf(idx))
                return l->state;
            break;
    }
    return {};
}

bool tree_model::hasChildren(QModelIndex const& parent) const
{
    return !!to_branch(parent);
}

bool tree_model::canFetchMore(QModelIndex const& parent) const
{
    auto b = to_branch(parent);
    return b && b->state == branch_state::blank;
}

Qt::ItemFlags tree_model::flags(QModelIndex const& idx) const
{
    auto ret = Qt::ItemIsEnabled | Qt::ItemIsSelectable;
    if (to_leaf(idx))
        ret |= Qt::ItemIsUserCheckable;
    return ret;
}

QModelIndex tree_model::index(int row, int, QModelIndex const& parent) const
{
    auto ptr = to_tree(parent);
    if (row < 0 || row >= static_cast<int>(ptr->children.size()))
        return {};
    return createIndex(row, 0, ptr->children[row].get());
}

QModelIndex tree_model::parent(QModelIndex const& idx) const
{
    if (!idx.isValid())
        return {};
    auto ptr = to_tree(idx)->parent;
    if (!ptr || ptr == root_.get())
        return {};
    auto& rows = ptr->parent->children;
    auto it =
        std::ranges::find_if(rows, [ptr](auto& p) { return p.get() == ptr; });
    if (it == rows.end())
        return {};
    auto row = static_cast<int>(std::distance(rows.begin(), it));
    return createIndex(row, 0, ptr);
}

int tree_model::rowCount(QModelIndex const& parent) const
{
    return static_cast<int>(to_tree(parent)->children.size());
}

bool tree_model::setData(QModelIndex const& idx, QVariant const&, int role)
{
    if (role != Qt::CheckStateRole)
        return false;
    if (auto l = to_leaf(idx)) {
        l->state = l->state == Qt::Checked ? Qt::Unchecked : Qt::Checked;
        emit dataChanged(idx, idx);
        return true;
    }
    return false;
}

bool tree_model::removeRows(int position, int rows, QModelIndex const& parent)
{
    if (rows <= 0)
        return true;
    beginRemoveRows(parent, position, position + rows - 1);
    auto& children = to_tree(parent)->children;
    children.erase(children.begin() + position,
                   children.begin() + position + rows);
    endRemoveRows();
    return true;
}

void tree_model::copy(QModelIndex const& idx)
{
    if (auto l = to_leaf(idx); l && !l->layer.raster)
        clipboard_ = *l;
}

void tree_model::on_pasted(QPersistentModelIndex per, leaf const& lyr)
{
    if (!per.isValid())
        return;
    if (auto b = to_branch(per); !b || b->state != branch_state::ready)
        return;
    auto ptr = to_tree(per);
    for (auto& ch : ptr->children)
        if (auto l = std::get_if<leaf>(&ch->data); l && l->layer == lyr.layer)
            return;
    int row = static_cast<int>(ptr->children.size());
    beginInsertRows(per, row, row);
    auto ch = std::make_unique<tree>(lyr);
    ch->parent = ptr;
    ptr->children.push_back(std::move(ch));
    endInsertRows();
}

void tree_model::on_dropped(QPersistentModelIndex per)
{
    if (!per.isValid() || !to_leaf(per))
        return;
    removeRows(per.row(), 1, per.parent());
}

void tree_model::on_fetch_canceled(QPersistentModelIndex per)
{
    if (!per.isValid())
        return;
    if (auto b = to_branch(per); b && b->state == branch_state::fetching) {
        b->state = branch_state::blank;
        emit dataChanged(per, per);
    }
}

void tree_model::on_fetched(  //
    QPersistentModelIndex per,
    std::vector<std::unique_ptr<tree>> children)
{
    if (!per.isValid())
        return;
    auto b = to_branch(per);
    if (!b || b->state != branch_state::fetching)
        return;
    b->state = branch_state::ready;
    if (int count = static_cast<int>(children.size())) {
        auto ptr = to_tree(per);
        for (auto& ch : children)
            ch->parent = ptr;
        beginInsertRows(per, 0, count - 1);
        ptr->children = std::move(children);
        endInsertRows();
    }
    else
        emit dataChanged(per, per);
}

void tree_model::request_stop()
{
    tasks_.request_stop();
    qInfo() << "stop requested";
}
