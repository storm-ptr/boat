// Andrew Naplavkov

#ifndef BOAT_GDAL_CATALOG_HPP
#define BOAT_GDAL_CATALOG_HPP

#include <boat/db/catalog.hpp>
#include <boat/db/query.hpp>
#include <boat/gdal/dataset.hpp>
#include <boat/gdal/detail/raster.hpp>
#include <boat/gdal/detail/vector.hpp>

namespace boat::gdal {

struct catalog : db::catalog {
    dataset_ptr dataset;

    std::vector<db::source> sources() override
    {
        auto ret = std::vector<db::source>{};
        auto meta = GDALGetMetadata(dataset.get(), "SUBDATASETS");
        if (!CSLCount(meta))
            return ret;
        for (int i = 1;; ++i) {
            auto desc = CSLFetchNameValue(
                meta, concat("SUBDATASET_", i, "_DESC").data());
            auto name = CSLFetchNameValue(
                meta, concat("SUBDATASET_", i, "_NAME").data());
            if (!desc || !name)
                break;
            ret.emplace_back(desc, name);
        }
        return ret;
    }

    std::vector<db::layer> layers() override
    {
        auto ret = std::vector<db::layer>{};
        if (GDALGetRasterCount(dataset.get()))
            ret.push_back({"", "_layer", "raster", true});
        ret.append_range(vectors(dataset.get()));
        return ret;
    }

    db::table get_table(std::string_view, std::string_view table_name) override
    {
        return gdal::get_table(GDALDatasetGetLayerByName(
            dataset.get(), std::string{table_name}.data()));
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

    void insert(db::table const& tbl, db::rowset const& rs) override
    {
        gdal::insert(
            GDALDatasetGetLayerByName(dataset.get(), tbl.table_name.data()),
            rs);
    }

    db::table create(db::table const& tbl) override
    {
        return gdal::get_table(add_table(dataset.get(), tbl));
    }

    void drop(std::string_view, std::string_view table_name) override
    {
        delete_table(dataset.get(), table_name);
    }

    db::raster get_raster(db::layer const& lyr) override
    {
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

#endif  // BOAT_GDAL_CATALOG_HPP
