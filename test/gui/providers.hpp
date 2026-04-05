// Andrew Naplavkov

#ifndef BOAT_TEST_GUI_PROVIDERS_HPP
#define BOAT_TEST_GUI_PROVIDERS_HPP

#include <boat/gdal/dal.hpp>
#include <boat/gui/caches/lru.hpp>
#include <boat/gui/provider.hpp>
#include <boat/slippy.hpp>
#include <boat/sql/dal.hpp>
#include "../commands.hpp"
#include "../data.hpp"

inline std::generator<std::shared_ptr<boat::db::dal>> dals()
{
    using namespace boat;
    auto tbl = get_table();
    auto objs = get_objects();
    for (auto cmd : commands()) {
        auto dal = std::make_shared<sql::dal>();
        dal->command = std::move(cmd);
        dal->command->set_autocommit(false);
        dal->drop(tbl.schema_name, tbl.table_name);
        dal->insert(dal->create(tbl), db::to_rowset(objs));
        co_yield std::move(dal);
    }
    {
        auto dal = std::make_shared<gdal::dal>();
        dal->dataset = gdal::create("", "mem");
        dal->insert(dal->create(tbl), db::to_rowset(objs));
        co_yield std::move(dal);
    }
    {
        auto dal = std::make_shared<gdal::dal>();
        dal->dataset = gdal::open(
            "/vsicurl/https://download.osgeo.org/gdal/data/gtiff/utm.tif");
        co_yield std::move(dal);
    }
    {
        auto dal = std::make_shared<slippy::dal>();
        dal->user = "useragent";
        dal->url = "http://basemaps.cartocdn.com/light_all/{z}/{x}/{y}.png";
        co_yield std::move(dal);
    }
}

inline std::generator<boat::gui::provider> providers()
{
    using namespace boat;
    auto cache = std::make_shared<gui::caches::lru>(10'000);
    auto key = size_t{};
    for (auto dal : dals()) {
        for (auto& lyr : dal->vectors())
            co_yield gui::provider{
                .dal = [=] -> decltype(auto) { return *dal; },
                .layer = std::move(lyr),
                .cache = cache,
                .key = ++key};
        for (auto& lyr : dal->rasters())
            co_yield gui::provider{
                .dal = [=] -> decltype(auto) { return *dal; },
                .layer = std::move(lyr),
                .cache = cache,
                .key = ++key};
    }
}

#endif  // BOAT_TEST_GUI_PROVIDERS_HPP
