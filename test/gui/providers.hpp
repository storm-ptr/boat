// Andrew Naplavkov

#ifndef BOAT_TEST_GUI_PROVIDERS_HPP
#define BOAT_TEST_GUI_PROVIDERS_HPP

#include <boat/gdal/catalog.hpp>
#include <boat/gui/caches/lru.hpp>
#include <boat/gui/provider.hpp>
#include <boat/slippy.hpp>
#include <boat/sql/catalog.hpp>
#include "../commands.hpp"
#include "../data.hpp"

inline std::generator<std::shared_ptr<boat::db::catalog>> catalogs()
{
    using namespace boat;
    auto tbl = get_table();
    auto objs = get_objects();
    for (auto cmd : commands()) {
        auto cat = std::make_shared<sql::catalog>();
        cat->command = std::move(cmd);
        cat->command->set_autocommit(false);
        cat->drop(tbl.schema_name, tbl.table_name);
        cat->insert(cat->create(tbl), db::to_rowset(objs));
        co_yield std::move(cat);
    }
    {
        auto cat = std::make_shared<gdal::catalog>();
        cat->dataset = gdal::create("", "mem");
        cat->insert(cat->create(tbl), db::to_rowset(objs));
        co_yield std::move(cat);
    }
    {
        auto cat = std::make_shared<gdal::catalog>();
        cat->dataset = gdal::open(
            "/vsicurl/https://download.osgeo.org/gdal/data/gtiff/utm.tif");
        co_yield std::move(cat);
    }
    {
        auto cat = std::make_shared<slippy::catalog>();
        cat->user = "useragent";
        cat->url = "http://basemaps.cartocdn.com/light_all/{z}/{x}/{y}.png";
        co_yield std::move(cat);
    }
}

inline std::generator<boat::gui::provider> providers()
{
    using namespace boat;
    auto cache = std::make_shared<gui::caches::lru>(10'000);
    auto key = size_t{};
    for (auto cat : catalogs())
        for (auto&& lyr : cat->layers())
            co_yield gui::provider{
                .catalog = [=] -> decltype(auto) { return *cat; },
                .layer = std::move(lyr),
                .cache = cache,
                .key = ++key};
}

#endif  // BOAT_TEST_GUI_PROVIDERS_HPP
