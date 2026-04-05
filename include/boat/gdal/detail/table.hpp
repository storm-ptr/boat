// Andrew Naplavkov

#ifndef BOAT_GDAL_TABLE_HPP
#define BOAT_GDAL_TABLE_HPP

#include <boat/db/meta.hpp>
#include <boat/gdal/detail/fields/fields.hpp>

namespace boat::gdal {

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

}  // namespace boat::gdal

#endif  // BOAT_GDAL_TABLE_HPP
