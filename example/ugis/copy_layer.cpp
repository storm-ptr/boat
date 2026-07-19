// Andrew Naplavkov

#include <QDebug>
#include <boat/catalogs.hpp>
#include <boat/gdal/catalog.hpp>
#include <boat/gui/caches/cache.hpp>
#include "copy_layer.h"

void copy_raster(  //
    leaf const& src,
    char const* dst,
    char const* drv,
    std::stop_token tok)
{
    if (tok.stop_requested())
        return;
    auto cat1 = boat::make_catalog(src.address);
    auto rast1 = cat1->get_raster(src.layer);
    if (tok.stop_requested())
        return;
    auto cat2 = boat::gdal::catalog{};
    cat2.dataset = boat::gdal::create(dst, drv, rast1);
    auto rast2 = cat2.get_raster(cat2.layers().at(0));
    auto z = boat::tile::zmax(rast1.width, rast1.height);
    auto done = size_t{};
    qInfo() << "copying tiles";
    for (auto chunk : boat::tile::all(rast1.width, rast1.height, z) |
                          std::views::chunk(100)) {
        auto batch = chunk | std::ranges::to<std::vector>();
        for (auto [tile, img] : cat1->read(rast1, batch)) {
            if (tok.stop_requested())
                return;
            cat2.write(rast2,
                       std::make_from_tuple<boat::db::rect>(
                           tile.rect(rast2.width, rast2.height)),
                       boost::gil::const_view(img));
        }
        done += batch.size();
        qInfo() << "copied" << done << "tiles";
    }
}

leaf copy_vector(  //
    leaf const& src,
    char const* dst,
    char const* drv,
    char const* name,
    std::stop_token tok)
{
    auto ret = src;
    ret.address = dst;
    ret.cache = boat::gui::caches::next_key();
    if (tok.stop_requested())
        return ret;
    auto cat1 = boat::make_catalog(src.address);
    auto tbl1 = cat1->get_table(src.layer.schema_name, src.layer.table_name);
    if (tok.stop_requested())
        return ret;
    auto cat2 = [&] -> std::unique_ptr<boat::db::catalog> {
        if (!drv)
            return boat::make_catalog(dst);
        auto ret = std::make_unique<boat::gdal::catalog>();
        ret->dataset = boat::gdal::create(dst, drv);
        return ret;
    }();
    auto tbl2 = tbl1;
    tbl2.schema_name.clear();
    tbl2.table_name = name;
    tbl2 = cat2->create(tbl2);
    ret.layer.schema_name = tbl2.schema_name;
    ret.layer.table_name = tbl2.table_name;
    cat2->set_autocommit(false);
    qInfo() << "copying rows";
    for (size_t done{};;) {
        auto rs =
            cat1->select(tbl1, boat::db::page{.offset = done, .limit = 20'000});
        if (rs.empty())
            break;
        cat2->insert(tbl2, rs, tok);
        if (tok.stop_requested())
            break;
        cat2->commit();
        done += rs.rows.size();
        qInfo() << "copied" << done << "rows";
    }
    cat2->set_autocommit(true);
    return ret;
}
