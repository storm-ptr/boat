// Andrew Naplavkov

#ifndef BOAT_GDAL_TABLE_HPP
#define BOAT_GDAL_TABLE_HPP

#include <boat/gdal/detail/fields/fields.hpp>
#include <boat/sql/reflection.hpp>

namespace boat::gdal {

constexpr auto to_column = overloaded{
    [](fields::attribute const& v) {
        return sql::column{
            .column_name = v.name,
            .lcase_type = to_lower(OGR_GetFieldTypeName(v.type)),
        };
    },
    [](fields::geometry const& v) {
        return sql::column{
            .column_name = v.name,
            .lcase_type = to_lower(OGRGeometryTypeToName(v.type)),
            .srid = v.epsg,
            .epsg = v.epsg,
        };
    },
};

inline OGRFieldType to_field_type(std::string_view type)
{
    if (type == sql::type<int64_t>)
        return OFTInteger64;
    if (type == sql::type<double>)
        return OFTReal;
    if (type == sql::type<std::string>)
        return OFTString;
    if (type == sql::type<blob>)
        return OFTBinary;
    if (type == sql::type<time_point>)
        return OFTDateTime;
    throw concat("OGRFieldType ", type);
}

inline sql::table get_table(OGRLayerH lyr)
{
    auto fd = OGR_L_GetLayerDefn(lyr);
    auto ret =
        sql::table{.lcase_dbms = "gdal", .table_name = OGR_FD_GetName(fd)};
    for (auto& fld : fields::make(fd))
        ret.columns.push_back(std::visit(to_column, fld));
    std::ranges::sort(ret.columns, {}, [](auto& col) {
        return col.column_name | unicode::utf32;
    });
    return ret;
}

inline OGRLayerH add_table(GDALDatasetH ds, sql::table const& tbl)
{
    auto ret = GDALDatasetCreateLayerFromGeomFieldDefn(
        ds, tbl.table_name.data(), 0, 0);
    auto types = sql::to_common_types(tbl);
    for (auto&& [tp, col] : std::views::zip(types, tbl.columns))
        if (col.epsg) {
            auto srs = make_epsg_srs(col.epsg);
            auto fld = unique_ptr<OGRGeomFieldDefnHS, OGR_GFld_Destroy>{
                OGR_GFld_Create(col.column_name.data(), wkbUnknown)};
            OGR_GFld_SetSpatialRef(fld.get(), srs.get());
            check(OGR_L_CreateGeomField(ret, fld.get(), 1));
        }
        else {
            auto fld = unique_ptr<void, OGR_Fld_Destroy>{
                OGR_Fld_Create(col.column_name.data(), to_field_type(tp))};
            check(OGR_L_CreateField(ret, fld.get(), 1));
        }
    return ret;
}

}  // namespace boat::gdal

#endif  // BOAT_GDAL_TABLE_HPP
