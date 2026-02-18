// Andrew Naplavkov

#ifndef BOAT_GDAL_ADAPTED_GEOMETRY_HPP
#define BOAT_GDAL_ADAPTED_GEOMETRY_HPP

#include <boat/blob.hpp>
#include <boat/gdal/detail/utility.hpp>

namespace boat::gdal {

inline blob get_geometry(OGRFeatureH feat, int index)
{
    auto geom = OGR_F_GetGeomFieldRef(feat, index);
    auto len = geom && !OGR_G_IsEmpty(geom) ? OGR_G_WkbSizeEx(geom) : 0u;
    auto ret = blob{len, std::byte{}};
    if (len)
        check(OGR_G_ExportToWkb(
            geom,
            std::endian::native == std::endian::little ? wkbNDR : wkbXDR,
            reinterpret_cast<unsigned char*>(ret.data())));
    return ret;
}

inline void set_geometry(OGRFeatureH feat, int index, blob_view wkb)
{
    auto geom = OGRGeometryH{};
    check(OGR_G_CreateFromWkbEx(wkb.data(), 0, &geom, wkb.size()));
    check(OGR_F_SetGeomFieldDirectly(feat, index, geom));
}

}  // namespace boat::gdal

#endif  // BOAT_GDAL_ADAPTED_GEOMETRY_HPP
