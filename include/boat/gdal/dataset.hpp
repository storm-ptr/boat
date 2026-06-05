// Andrew Naplavkov

#ifndef BOAT_GDAL_DATASET_HPP
#define BOAT_GDAL_DATASET_HPP

#include <boat/db/meta.hpp>
#include <boat/gdal/detail/utility.hpp>

namespace boat::gdal {

inline dataset_ptr open(char const* file)
{
    init();
    auto ret = dataset_ptr{GDALOpenEx(file, 0, 0, 0, 0)};
    boat::check(!!ret, file);
    return ret;
}

inline dataset_ptr create(char const* file, char const* driver)
{
    init();
    auto drv = GDALGetDriverByName(driver);
    boat::check(!!drv, driver);
    auto ret = dataset_ptr{GDALCreate(drv, file, 0, 0, 0, GDT_Unknown, 0)};
    boat::check(!!ret, "GDALCreate");
    return ret;
}

inline dataset_ptr create(  //
    char const* file,
    char const* driver,
    db::raster const& rast)
{
    init();
    auto drv = GDALGetDriverByName(driver);
    boat::check(!!drv, driver);
    auto ret = dataset_ptr{GDALCreate(  //
        drv,
        file,
        rast.width,
        rast.height,
        static_cast<int>(rast.bands.size()),
        GDALGetDataTypeByName(rast.bands.at(0).type_name.data()),
        0)};
    boat::check(!!ret, "GDALCreate");
    auto a = std::array{
        rast.xorig,
        rast.xscale,
        rast.xskew,
        rast.yorig,
        rast.yskew,
        rast.yscale,
    };
    check(GDALSetGeoTransform(ret.get(), a.data()));
    auto crs = make_srs(rast.epsg, rast.wkt, rast.proj4);
    check(GDALSetSpatialRef(ret.get(), crs.release()));
    return ret;
}

}  // namespace boat::gdal

#endif  // BOAT_GDAL_DATASET_HPP
