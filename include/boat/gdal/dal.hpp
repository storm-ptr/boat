// Andrew Naplavkov

#ifndef BOAT_GDAL_DAL_HPP
#define BOAT_GDAL_DAL_HPP

#include <boat/db/dal.hpp>
#include <boat/db/query.hpp>
#include <boat/gdal/dataset.hpp>
#include <boat/gdal/detail/raster.hpp>
#include <boat/gdal/detail/vector.hpp>

namespace boat::gdal {

struct dal : db::dal {
    dataset_ptr dataset;

    std::vector<db::layer> vectors() override
    {
        return gdal::vectors(dataset.get());
    }

    db::table get_table(std::string_view, std::string_view table_name) override
    {
        if (auto lyr = GDALDatasetGetLayerByName(
                dataset.get(), std::string{table_name}.data()))
            return gdal::get_table(lyr);
        return {};
    }

    db::rowset select(db::table const& tbl, db::page const& rq) override
    {
        auto q = db::query{"\n select * from ", db::id{tbl.table_name}};
        for (auto sep{"\n order by "}; auto& key : rq.order_by)
            q << std::exchange(sep, ", ") << db::id{key.column_name}
              << (key.descending ? " desc" : "");
        q << "\n limit " << to_chars(rq.limit) << "\n offset "
          << to_chars(rq.offset);
        auto lyr = execute(dataset.get(), q.text('"', {}).data(), 0);
        auto fd = OGR_L_GetLayerDefn(lyr.get());
        return gdal::select(
            lyr.get(), fields::make(fd, rq.select_list), rq.limit);
    }

    db::rowset select(db::table const& tbl, db::bbox const& rq) override
    {
        auto lyr =
            GDALDatasetGetLayerByName(dataset.get(), tbl.table_name.data());
        auto fd = OGR_L_GetLayerDefn(lyr);
        OGR_L_SetSpatialFilterRectEx(
            lyr,
            std::max<>(0, OGR_FD_GetGeomFieldIndex(fd, rq.layer_column.data())),
            rq.xmin,
            rq.ymin,
            rq.xmax,
            rq.ymax);
        return gdal::select(lyr, fields::make(fd, rq.select_list), rq.limit);
    }

    void insert(db::table const& tbl, db::rowset const& vals) override
    {
        gdal::insert(
            GDALDatasetGetLayerByName(dataset.get(), tbl.table_name.data()),
            vals);
    }

    db::table create(db::table const& tbl) override
    {
        return gdal::get_table(add_table(dataset.get(), tbl));
    }

    void drop(std::string_view, std::string_view table_name) override
    {
        delete_table(dataset.get(), table_name);
    }

    std::vector<db::layer> rasters() override
    {
        if (!GDALGetRasterCount(dataset.get()))
            return {};
        return {{.table_name = "_layer", .column_name = "_raster"}};
    }

    db::raster get_raster(db::layer const& lyr) override
    {
        if (!GDALGetRasterCount(dataset.get()))
            return {};
        return gdal::get_raster(dataset.get());
    }

    std::generator<std::pair<tile, blob>> mosaic(  //
        db::raster r,
        std::vector<tile> ts) override
    {
        for (auto& t : ts)
            co_yield {t, get_png(dataset.get(), r, t)};
    }
};

}  // namespace boat::gdal

#endif  // BOAT_GDAL_DAL_HPP
