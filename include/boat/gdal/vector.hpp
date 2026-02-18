// Andrew Naplavkov

#ifndef BOAT_GDAL_VECTOR_HPP
#define BOAT_GDAL_VECTOR_HPP

#include <boat/db/query.hpp>
#include <boat/gdal/dataset.hpp>
#include <boat/gdal/detail/rowset.hpp>
#include <boat/gdal/detail/table.hpp>

namespace boat::gdal {

inline std::vector<sql::layer> get_layers(GDALDatasetH ds)
{
    auto ret = std::vector<sql::layer>{};
    for (int i = 0, n = GDALDatasetGetLayerCount(ds); i < n; ++i) {
        auto fd = OGR_L_GetLayerDefn(GDALDatasetGetLayer(ds, i));
        for (int j = 0, m = OGR_FD_GetGeomFieldCount(fd); j < m; ++j)
            ret.push_back({.table_name = OGR_FD_GetName(fd),
                           .column_name = OGR_GFld_GetNameRef(
                               OGR_FD_GetGeomFieldDefn(fd, j))});
    }
    std::ranges::sort(ret, {}, [](auto& lyr) {
        return std::tuple{
            lyr.table_name | unicode::utf32,
            lyr.column_name | unicode::utf32,
        };
    });
    return ret;
}

inline sql::table describe(GDALDatasetH ds, char const* table_name)
{
    return get_table(GDALDatasetGetLayerByName(ds, table_name));
}

inline pfr::rowset select(  //
    GDALDatasetH ds,
    sql::table const& tbl,
    sql::page const& req)
{
    auto q = db::query{};
    q << "\n select * from " << db::id{tbl.table_name};
    for (auto sep = "\n order by "; auto& key : req.order_by)
        q << std::exchange(sep, ", ") << db::id{key.column_name}
          << (key.descending ? " desc" : "");
    q << "\n limit " << to_chars(req.limit) << "\n offset "
      << to_chars(req.offset);
    auto lyr = execute(ds, q.sql('"', {}).data(), 0);
    auto fd = OGR_L_GetLayerDefn(lyr.get());
    return read(lyr.get(), fields::make(fd, req.select_list), req.limit);
}

inline pfr::rowset select(  //
    GDALDatasetH ds,
    sql::table const& tbl,
    sql::bbox const& req)
{
    auto lyr = GDALDatasetGetLayerByName(ds, tbl.table_name.data());
    auto fd = OGR_L_GetLayerDefn(lyr);
    OGR_L_SetSpatialFilterRectEx(
        lyr,
        std::max<>(0, OGR_FD_GetGeomFieldIndex(fd, req.spatial_column.data())),
        req.xmin,
        req.ymin,
        req.xmax,
        req.ymax);
    return read(lyr, fields::make(fd, req.select_list), req.limit);
}

inline void insert(  //
    GDALDatasetH ds,
    sql::table const& tbl,
    pfr::rowset const& vals)
{
    write(GDALDatasetGetLayerByName(ds, tbl.table_name.data()), vals);
}

inline sql::table create(GDALDatasetH ds, sql::table const& tbl)
{
    return get_table(add_table(ds, tbl));
}

}  // namespace boat::gdal

#endif  // BOAT_GDAL_VECTOR_HPP
