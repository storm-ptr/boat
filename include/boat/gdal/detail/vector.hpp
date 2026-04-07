// Andrew Naplavkov

#ifndef BOAT_GDAL_VECTOR_HPP
#define BOAT_GDAL_VECTOR_HPP

#include <boat/db/meta.hpp>
#include <boat/db/rowset.hpp>
#include <boat/gdal/detail/fields/fields.hpp>

namespace boat::gdal {

inline std::vector<db::layer> vectors(GDALDatasetH ds)
{
    auto ret = std::vector<db::layer>{};
    for (int i{}, n = GDALDatasetGetLayerCount(ds); i < n; ++i) {
        auto fd = OGR_L_GetLayerDefn(GDALDatasetGetLayer(ds, i));
        for (int j{}, m = OGR_FD_GetGeomFieldCount(fd); j < m; ++j)
            ret.push_back({.table_name = OGR_FD_GetName(fd),
                           .column_name = OGR_GFld_GetNameRef(
                               OGR_FD_GetGeomFieldDefn(fd, j))});
    }
    return ret;
}

constexpr auto to_column = overloaded{
    [](fields::attribute const& v) {
        return db::column{
            .kind{v.kind()},
            .column_name = v.name,
            .type_name = to_lower(OGR_GetFieldTypeName(v.type)),
        };
    },
    [](fields::geometry const& v) {
        return db::column{
            .kind{v.kind},
            .column_name = v.name,
            .type_name = to_lower(OGRGeometryTypeToName(v.type)),
            .srid = v.epsg,
            .epsg = v.epsg,
        };
    },
};

inline db::table get_table(OGRLayerH lyr)
{
    auto fd = OGR_L_GetLayerDefn(lyr);
    auto ret = db::table{.dbms = "gdal", .table_name = OGR_FD_GetName(fd)};
    for (auto& fld : fields::make(fd))
        ret.columns.push_back(std::visit(to_column, fld));
    return ret;
}

inline OGRLayerH add_table(GDALDatasetH ds, db::table const& tbl)
{
    auto ret = GDALDatasetCreateLayerFromGeomFieldDefn(
        ds, tbl.table_name.data(), 0, 0);
    for (auto& col : tbl.columns)
        if (col.epsg) {
            auto sys = make_epsg_srs(col.epsg);
            auto fld = unique_ptr<OGRGeomFieldDefnHS, OGR_GFld_Destroy>{
                OGR_GFld_Create(col.column_name.data(), wkbUnknown)};
            OGR_GFld_SetSpatialRef(fld.get(), sys.get());
            check(OGR_L_CreateGeomField(ret, fld.get(), 1));
        }
        else {
            auto fld = unique_ptr<void, OGR_Fld_Destroy>{OGR_Fld_Create(
                col.column_name.data(), fields::to_type(col.kind))};
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

inline void insert(OGRLayerH lyr, db::rowset const& vals)
{
    auto fd = OGR_L_GetLayerDefn(lyr);
    auto flds = fields::make(fd, vals.columns);
    for (auto const& row : vals) {
        auto feat = feature_ptr{OGR_F_Create(fd)};
        for (auto&& [fld, var] : std::views::zip(flds, row))
            std::visit([&](auto& v) { v.write(feat.get(), var); }, fld);
        check(OGR_L_CreateFeature(lyr, feat.get()));
    }
}

}  // namespace boat::gdal

#endif  // BOAT_GDAL_VECTOR_HPP
