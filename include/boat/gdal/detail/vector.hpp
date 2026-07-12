// Andrew Naplavkov

#ifndef BOAT_GDAL_VECTOR_HPP
#define BOAT_GDAL_VECTOR_HPP

#include <boat/db/rowset.hpp>
#include <boat/gdal/detail/fields/fields.hpp>
#include <stop_token>

namespace boat::gdal {

inline std::vector<db::layer> vectors(GDALDatasetH ds)
{
    auto ret = std::vector<db::layer>{};
    for (int i{}, n = GDALDatasetGetLayerCount(ds); i < n; ++i)
        ret.append_range(
            fields::to_layers(OGR_L_GetLayerDefn(GDALDatasetGetLayer(ds, i))));
    return ret;
}

inline db::table get_table(OGRLayerH lyr)
{
    auto ret = db::table{
        .dbms = to_lower(GDALGetDriverShortName(
            GDALGetDatasetDriver(OGR_L_GetDataset(lyr)))),
        .table_name = OGR_FD_GetName(OGR_L_GetLayerDefn(lyr)),
    };
    auto to_index_key = fields::to_index_key(lyr);
    for (auto& fld : fields::make(lyr)) {
        ret.columns.push_back(std::visit(fields::to_column, fld));
        if (auto idx = std::visit(to_index_key, fld))
            ret.index_keys.push_back(*std::move(idx));
    }
    return ret;
}

inline OGRLayerH add_table(GDALDatasetH ds, db::table const& tbl)
{
    OGRLayerH ret = 0;
    for (auto& col : tbl.columns) {
        if (!col.has_coord_sys())
            continue;
        auto crs = make_srs(col.epsg, col.wkt, col.proj4);
        auto fld = unique_ptr<OGRGeomFieldDefnHS, OGR_GFld_Destroy>{
            OGR_GFld_Create(col.column_name.data(), wkbUnknown)};
        OGR_GFld_SetSpatialRef(fld.get(), crs.get());
        if (ret)
            check(OGR_L_CreateGeomField(ret, fld.get(), 1));
        else if (GDALDatasetTestCapability(
                     ds, ODsCCreateGeomFieldAfterCreateLayer)) {
            ret = GDALDatasetCreateLayerFromGeomFieldDefn(
                ds, tbl.table_name.data(), 0, 0);
            check(OGR_L_CreateGeomField(ret, fld.get(), 1));
        }
        else {
            auto name = concat("GEOMETRY_NAME=", col.column_name);
            char const* opts[] = {name.data(), "SPATIAL_INDEX=YES", nullptr};
            ret = GDALDatasetCreateLayerFromGeomFieldDefn(
                ds, tbl.table_name.data(), fld.get(), opts);
        }
    }
    boat::check(!!ret, "no geometry");
    for (auto& col : tbl.columns) {
        if (col.has_coord_sys())
            continue;
        auto fld = unique_ptr<void, OGR_Fld_Destroy>{
            OGR_Fld_Create(col.column_name.data(), fields::to_type(col.kind))};
        check(OGR_L_CreateField(ret, fld.get(), 1));
    }
    return ret;
}

inline void delete_table(GDALDatasetH ds, std::string_view tbl)
{
    for (int i{}, n = GDALDatasetGetLayerCount(ds); i < n; ++i) {
        auto lyr = GDALDatasetGetLayer(ds, i);
        auto fd = OGR_L_GetLayerDefn(lyr);
        if (tbl == OGR_FD_GetName(fd)) {
            check(OGR_DS_DeleteLayer(ds, i));
            break;
        }
    }
}

db::rowset select(OGRLayerH lyr, range_of<fields::field> auto&& flds, int limit)
{
    auto ret = db::rowset{};
    for (auto& fld : flds)
        ret.columns.push_back(std::visit([&](auto& v) { return v.name; }, fld));
    for (int i = 0; i < limit; ++i) {
        auto feat = feature_ptr{OGR_L_GetNextFeature(lyr)};
        if (!feat)
            break;
        for (auto& row = ret.rows.emplace_back(); auto& fld : flds)
            row.push_back(
                std::visit([&](auto& v) { return v.read(feat.get()); }, fld));
    }
    return ret;
}

inline void insert(OGRLayerH lyr, db::rowset const& rs, std::stop_token tok)
{
    auto flds = fields::make(lyr, rs.columns);
    auto fd = OGR_L_GetLayerDefn(lyr);
    for (auto const& row : rs) {
        if (tok.stop_requested())
            break;
        auto feat = feature_ptr{OGR_F_Create(fd)};
        for (auto&& [fld, var] : std::views::zip(flds, row))
            std::visit([&](auto& v) { v.write(feat.get(), var); }, fld);
        check(OGR_L_CreateFeature(lyr, feat.get()));
    }
}

}  // namespace boat::gdal

#endif  // BOAT_GDAL_VECTOR_HPP
