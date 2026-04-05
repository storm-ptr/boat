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
    db::raster const& r)
{
    init();
    auto drv = GDALGetDriverByName(driver);
    boat::check(!!drv, driver);
    auto ret = dataset_ptr{GDALCreate(  //
        drv,
        file,
        r.width,
        r.height,
        static_cast<int>(r.bands.size()),
        GDALGetDataTypeByName(r.bands.at(0).type_name.data()),
        0)};
    boat::check(!!ret, "GDALCreate");
    auto a = std::array{r.xorig, r.xscale, r.xskew, r.yorig, r.yskew, r.yscale};
    check(GDALSetGeoTransform(ret.get(), a.data()));
    check(GDALSetSpatialRef(ret.get(), make_epsg_srs(r.epsg).get()));
    return ret;
}

}  // namespace boat::gdal

#endif  // BOAT_GDAL_DATASET_HPP
