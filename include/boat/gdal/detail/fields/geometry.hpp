// Andrew Naplavkov

#ifndef BOAT_GDAL_FIELDS_GEOMETRY_HPP
#define BOAT_GDAL_FIELDS_GEOMETRY_HPP

#include <boat/db/adapted/adapted.hpp>
#include <boat/gdal/detail/adapted/geometry.hpp>

namespace boat::gdal::fields {

struct geometry {
    static constexpr auto kind =
        db::kind<boat::geometry::geographic::variant>::value;
    std::string name;
    OGRwkbGeometryType type;
    int epsg;
    int index;

    static geometry make(OGRFeatureDefnH fd, int index)
    {
        auto fld = OGR_FD_GetGeomFieldDefn(fd, index);
        return {.name = OGR_GFld_GetNameRef(fld),
                .type = OGR_GFld_GetType(fld),
                .epsg = get_authority_code(OGR_GFld_GetSpatialRef(fld)),
                .index = index};
    }

    db::variant read(OGRFeatureH feat) const
    {
        auto wkb = get_geometry(feat, index);
        return wkb.empty() ? db::variant{} : db::variant{std::move(wkb)};
    }

    void write(OGRFeatureH feat, db::variant const& var) const
    {
        if (var)
            set_geometry(feat, index, std::get<blob>(var));
    }
};

}  // namespace boat::gdal::fields

#endif  // BOAT_GDAL_FIELDS_GEOMETRY_HPP
