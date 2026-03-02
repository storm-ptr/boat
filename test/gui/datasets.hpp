// Andrew Naplavkov

#ifndef BOAT_TEST_GUI_DATASETS_HPP
#define BOAT_TEST_GUI_DATASETS_HPP

#include <boat/gui/caches/lru.hpp>
#include <boat/gui/datasets/datasets.hpp>
#include "../commands.hpp"
#include "../data.hpp"

inline std::generator<std::shared_ptr<boat::gui::datasets::dataset>> datasets()
{
    using namespace boat;

    class ref : public db::agent {
        db::agent& agt_;

    public:
        explicit ref(db::agent& agt) : agt_(agt) {}

        std::vector<db::layer> get_layers() override
        {
            return agt_.get_layers();
        }

        db::table describe(std::string_view schema_name,
                           std::string_view table_name) override
        {
            return agt_.describe(schema_name, table_name);
        }

        db::rowset select(db::table const& tbl, db::page const& rq) override
        {
            return agt_.select(tbl, rq);
        }

        db::rowset select(db::table const& tbl, db::bbox const& rq) override
        {
            return agt_.select(tbl, rq);
        }

        void insert(db::table const& tbl, db::rowset const& vals) override
        {
            agt_.insert(tbl, vals);
        }

        db::table create(db::table const& tbl) override
        {
            return agt_.create(tbl);
        }

        void drop(std::string_view schema_name,
                  std::string_view table_name) override
        {
            agt_.drop(schema_name, table_name);
        }
    };

    auto cache = std::make_shared<gui::caches::lru>(10'000);
    auto tbl = get_object_table();
    auto objs = get_objects();
    for (auto cmd : commands()) {
        auto agt = sql::agent{};
        agt.command = std::move(cmd);
        agt.command->set_autocommit(false);
        agt.drop(tbl.schema_name, tbl.table_name);
        agt.insert(agt.create(tbl), db::to_rowset(objs));
        co_yield std::make_shared<gui::datasets::db>(
            [agt = std::move(agt)] mutable {
                return std::make_unique<ref>(agt);
            },
            cache);
    }
    {
        auto agt = gdal::agent{};
        agt.dataset = gdal::create("", "mem");
        agt.insert(agt.create(tbl), db::to_rowset(objs));
        co_yield std::make_shared<gui::datasets::db>(
            [agt = std::move(agt)] mutable {
                return std::make_unique<ref>(agt);
            },
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
