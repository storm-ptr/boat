// Andrew Naplavkov

#ifndef BOAT_GDAL_FIELDS_HPP
#define BOAT_GDAL_FIELDS_HPP

#include <boat/db/meta.hpp>
#include <boat/gdal/detail/fields/attribute.hpp>
#include <boat/gdal/detail/fields/fid.hpp>
#include <boat/gdal/detail/fields/geometry.hpp>

namespace boat::gdal::fields {

using field = std::variant<fid, attribute, geometry>;

constexpr auto to_column = overloaded{
    [](fid const& v) {
        return db::column{
            .kind{v.kind},
            .column_name = v.name,
            .type_name = to_lower(OGR_GetFieldTypeName(v.type)),
        };
    },
    [](attribute const& v) {
        return db::column{
            .kind{v.kind()},
            .column_name = v.name,
            .type_name = to_lower(OGR_GetFieldTypeName(v.type)),
        };
    },
    [](geometry const& v) {
        return db::column{
            .kind{v.kind},
            .column_name = v.name,
            .type_name = to_lower(OGRGeometryTypeToName(v.type)),
            .srid = v.epsg,
            .epsg = v.epsg,
            .wkt = v.wkt,
            .proj4 = v.proj4,
        };
    },
};

inline auto to_index_key(OGRLayerH lyr)
{
    return overloaded{
        [](fid const& v) -> std::optional<db::index_key> {
            return db::index_key{
                .index_name = "pk",
                .column_name = v.name,
                .primary = true,
                .ordinal = 1,
            };
        },
        [](attribute const& v) -> std::optional<db::index_key> {
            return std::nullopt;
        },
        [=](geometry const& v) -> std::optional<db::index_key> {
            if (!OGR_L_TestCapability(lyr, OLCFastSpatialFilter))
                return std::nullopt;
            return db::index_key{
                .index_name = concat("idx_", v.name),
                .column_name = v.name,
                .ordinal = 1,
            };
        },
    };
}

inline std::vector<db::layer> to_layers(OGRFeatureDefnH fd)
{
    auto ret = std::vector<db::layer>{};
    for (int i{}, m = OGR_FD_GetGeomFieldCount(fd); i < m; ++i) {
        auto lyr = db::layer{
            .table_name = OGR_FD_GetName(fd),
            .column_name = OGR_GFld_GetNameRef(OGR_FD_GetGeomFieldDefn(fd, i))};
        if (lyr.column_name.empty())
            lyr.column_name = geometry::alias;
        ret.push_back(std::move(lyr));
    }
    return ret;
}

inline field make(OGRLayerH lyr, char const* name)
{
    if (auto fld = fid::make(lyr); fld && fld->name == name)
        return *std::move(fld);
    auto fd = OGR_L_GetLayerDefn(lyr);
    if (auto i = OGR_FD_GetFieldIndex(fd, name); i >= 0)
        return attribute::make(fd, i);
    if (auto i = OGR_FD_GetGeomFieldIndex(fd, name); i >= 0)
        return geometry::make(fd, i);
    if (auto fld = geometry::make(fd, 0); fld.name == geometry::alias) {
        fld.name = name;
        return fld;
    }
    throw std::runtime_error(concat("no field ", name));
}

inline std::vector<field> make(  //
    OGRLayerH lyr,
    std::vector<std::string> const& cols = {})
{
    if (cols.empty()) {
        auto ret = std::vector<field>{};
        if (auto fld = fid::make(lyr))
            ret.push_back(*std::move(fld));
        auto fd = OGR_L_GetLayerDefn(lyr);
        for (int i = 0, n = OGR_FD_GetFieldCount(fd); i < n; ++i)
            ret.push_back(attribute::make(fd, i));
        for (int i = 0, n = OGR_FD_GetGeomFieldCount(fd); i < n; ++i)
            ret.push_back(geometry::make(fd, i));
        return ret;
    }
    auto fn = [&](auto& col) { return make(lyr, col.data()); };
    return cols | std::views::transform(fn) | std::ranges::to<std::vector>();
}

}  // namespace boat::gdal::fields

#endif  // BOAT_GDAL_FIELDS_HPP
