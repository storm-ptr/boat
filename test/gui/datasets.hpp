// Andrew Naplavkov

#ifndef BOAT_TEST_GUI_DATASETS_HPP
#define BOAT_TEST_GUI_DATASETS_HPP

#include <boat/gui/caches/lru.hpp>
#include <boat/gui/datasets/datasets.hpp>
#include <boat/sql/reflection.hpp>
#include "../commands.hpp"
#include "../data.hpp"

inline std::generator<std::shared_ptr<boat::gui::datasets::dataset>> datasets()
{
    using namespace boat;

    class ref : public db::command {
        db::command* ptr_;

    public:
        ref(db::command* ptr) : ptr_(ptr) {}
        pfr::rowset exec(db::query const& q) override { return ptr_->exec(q); };
        void set_autocommit(bool on) override { ptr_->set_autocommit(on); };
        void commit() override { ptr_->commit(); };
        char id_quote() override { return ptr_->id_quote(); };
        std::string param_mark() override { return ptr_->param_mark(); };
        std::string dbms() override { return ptr_->dbms(); };
    };

    static auto cache = std::make_shared<gui::caches::lru>(10'000);

    for (auto cmd : commands()) {
        cmd->set_autocommit(false);
        cmd->exec({"drop table if exists ", db::id{object_table.table_name}});
        auto tbl = sql::create(*cmd, object_table);
        sql::insert(*cmd, tbl, pfr::to_rowset(objects));
        co_yield std::make_shared<gui::datasets::sql>(
            [cmd = std::move(cmd)] { return std::make_unique<ref>(cmd.get()); },
            cache);
    }
    co_yield std::make_shared<gui::datasets::slippy>("algol", cache);
}

#endif  // BOAT_TEST_GUI_DATASETS_HPP
