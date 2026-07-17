// Andrew Naplavkov

#ifndef BOAT_GDAL_DATASET_HPP
#define BOAT_GDAL_DATASET_HPP

#include <boat/db/meta.hpp>
#include <boat/gdal/detail/utility.hpp>

namespace boat::gdal {

inline dataset_ptr open(char const* file)
{
    init();
    char const* opts[] = {"MSSQLSPATIAL_USE_BCP=NO", nullptr};
    auto ret = dataset_ptr{GDALOpenEx(file, 0, 0, 0, opts)};
    boat::check(!!ret, error_or(concat("GDALOpenEx ", file)));
    return ret;
}

inline dataset_ptr create(char const* file, char const* driver)
{
    init();
    char const* opts[] = {"SPATIALITE=YES", nullptr};
    auto drv = GDALGetDriverByName(driver);
    boat::check(!!drv, error_or(concat("GDALGetDriverByName ", driver)));
    auto ret = dataset_ptr{GDALCreate(drv, file, 0, 0, 0, GDT_Unknown, opts)};
    boat::check(!!ret, error_or(concat("GDALCreate ", file)));
    return ret;
}

inline dataset_ptr create(  //
    char const* file,
    char const* driver,
    db::raster const& rast)
{
    init();
    auto drv = GDALGetDriverByName(driver);
    boat::check(!!drv, error_or(concat("GDALGetDriverByName ", driver)));
    auto ret = dataset_ptr{GDALCreate(  //
        drv,
        file,
        rast.width,
        rast.height,
        static_cast<int>(rast.bands.size()),
        GDALGetDataTypeByName(rast.bands.at(0).type_name.data()),
        0)};
    boat::check(!!ret, error_or(concat("GDALCreate ", file)));
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
    check(GDALSetSpatialRef(ret.get(), crs.get()));
    for (int i{}, n = static_cast<int>(rast.bands.size()); i < n; ++i) {
        auto b = GDALGetRasterBand(ret.get(), i + 1);
        auto ci =
            GDALGetColorInterpretationByName(rast.bands[i].color_name.data());
        boat::check(ci != GCI_Undefined, rast.bands[i].color_name);
        check(GDALSetRasterColorInterpretation(b, ci));
    }
    return ret;
}

}  // namespace boat::gdal

#endif  // BOAT_GDAL_DATASET_HPP
