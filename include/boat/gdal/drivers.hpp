// Andrew Naplavkov

#ifndef BOAT_GDAL_DRIVERS_HPP
#define BOAT_GDAL_DRIVERS_HPP

#if __has_include(<gdal.h>)
#include <boat/gdal/detail/utility.hpp>
#else
#include <string>
#endif
#include <generator>

namespace boat::gdal {

struct driver {
    std::string short_name;
    std::string long_name;
    std::string extensions;
};

enum class driver_type { Raster, Vector };
enum class driver_op { Create, Open };

inline std::generator<driver> drivers(  //
    [[maybe_unused]] driver_type type,
    [[maybe_unused]] driver_op op)
{
#if __has_include(<gdal.h>)
    init();
    for (int i{}, n = GDALGetDriverCount(); i < n; ++i) {
        auto drv = GDALGetDriver(i);
        if (!GDALGetMetadataItem(  //
                drv,
                type == driver_type::Raster ? GDAL_DCAP_RASTER
                                            : GDAL_DCAP_VECTOR,
                0))
            continue;
        if (!GDALGetMetadataItem(
                drv,
                op == driver_op::Create ? GDAL_DCAP_CREATE : GDAL_DCAP_OPEN,
                0))
            continue;
        auto short_name = GDALGetDriverShortName(drv);
        if (!short_name || !*short_name)
            continue;
        auto long_name = GDALGetDriverLongName(drv);
        if (!long_name || !*long_name)
            continue;
        auto extensions = GDALGetMetadataItem(drv, GDAL_DMD_EXTENSIONS, 0);
        if (!extensions || !*extensions)
            continue;
        co_yield {short_name, long_name, extensions};
    }
#else
    co_return;
#endif
}

}  // namespace boat::gdal

#endif  // BOAT_GDAL_DRIVERS_HPP
