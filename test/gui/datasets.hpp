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
        db::command& cmd_;

    public:
        ref(db::command& cmd) : cmd_(cmd) {}
        pfr::rowset exec(db::query const& q) override { return cmd_.exec(q); };
        void set_autocommit(bool on) override { cmd_.set_autocommit(on); };
        void commit() override { cmd_.commit(); };
        char id_quote() override { return cmd_.id_quote(); };
        std::string param_mark() override { return cmd_.param_mark(); };
        std::string lcase_dbms() override { return cmd_.lcase_dbms(); };
    };

    auto cache = std::make_shared<gui::caches::lru>(10'000);
    auto draft = get_object_table();
    auto objs = get_objects();
    for (auto cmd : commands()) {
        cmd->set_autocommit(false);
        cmd->exec({"drop table if exists ", db::id{draft.table_name}});
        auto tbl = sql::create(*cmd, draft);
        auto rows = pfr::to_rowset(objs);
        sql::insert(*cmd, tbl, rows);
        co_yield std::make_shared<gui::datasets::sql>(
            [cmd = std::move(cmd)] { return std::make_unique<ref>(*cmd); },
            cache);
    }
    co_yield std::make_shared<gui::datasets::slippy>(
        "useragent",
        "http://basemaps.cartocdn.com/light_all/{z}/{x}/{y}.png",
        cache);
    co_yield std::make_shared<gui::datasets::gdal>(
        "/vsicurl/https://download.osgeo.org/gdal/data/gtiff/utm.tif", cache);
}

#endif  // BOAT_TEST_GUI_DATASETS_HPP
