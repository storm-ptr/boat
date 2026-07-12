// Andrew Naplavkov

#ifndef TREE_MODEL_H
#define TREE_MODEL_H

#include <QAbstractItemModel>
#include <QString>
#include <concepts>
#include <optional>
#include "task_group.h"
#include "tree.h"

class tree_model : public QAbstractItemModel {
public:
    explicit tree_model(QObject* parent);
    bool busy() { return tasks_.busy(); }
    bool can_paste_to(QModelIndex const&) const;
    bool can_refresh(QModelIndex const&) const;
    QString clipboard_name() const;
    void copy(QModelIndex const&);
    void copy_as(  //
        QModelIndex const&,
        QString const& path,
        QString const& driver);
    void describe(QModelIndex const&);
    void drop(QModelIndex const&);
    std::optional<leaf> get_leaf(QModelIndex const&) const;
    bool is_branch(QModelIndex const&) const;
    bool is_mounted(QModelIndex const&) const;
    void mount(boat::db::source const&);
    void new_workspace();
    bool open_workspace(QString const& path);
    void paste(QModelIndex const&, QString const& name);
    void request_stop();
    void refresh(QModelIndex const&);
    bool save_workspace(QString const& path) const;
    void unmount(QModelIndex const&);

    void mutate_leaf(QModelIndex const& idx, std::invocable<leaf&> auto&& fn)
    {
        if (auto l = to_leaf(idx); l && !l->layer.raster) {
            std::invoke(fn, *l);
            emit dataChanged(idx, idx);
        }
    }

    int columnCount(QModelIndex const&) const override { return 1; }
    QVariant data(QModelIndex const&, int = Qt::DisplayRole) const override;
    void fetchMore(QModelIndex const&) override;
    Qt::ItemFlags flags(QModelIndex const&) const override;
    bool hasChildren(QModelIndex const&) const override;
    bool canFetchMore(QModelIndex const&) const override;
    QModelIndex index(int, int, QModelIndex const& = {}) const override;
    QModelIndex parent(QModelIndex const&) const override;
    bool removeRows(int, int, QModelIndex const& = {}) override;
    int rowCount(QModelIndex const& = {}) const override;
    bool setData(  //
        QModelIndex const&,
        QVariant const&,
        int = Qt::EditRole) override;

private:
    void on_dropped(QPersistentModelIndex);
    void on_fetch_canceled(QPersistentModelIndex);
    void on_fetched(QPersistentModelIndex, std::vector<std::unique_ptr<tree>>);
    void on_pasted(QPersistentModelIndex, leaf const&);
    tree* to_tree(QModelIndex const&) const;
    branch* to_branch(QModelIndex const&) const;
    leaf* to_leaf(QModelIndex const&) const;

    std::unique_ptr<tree> root_;
    std::optional<leaf> clipboard_;
    task_group tasks_;
};

#endif  // TREE_MODEL_H
