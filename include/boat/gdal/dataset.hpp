// Andrew Naplavkov

#ifndef BOAT_GDAL_DATASET_HPP
#define BOAT_GDAL_DATASET_HPP

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

}  // namespace boat::gdal

#endif  // BOAT_GDAL_DATASET_HPP
