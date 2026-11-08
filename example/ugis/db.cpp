// Andrew Naplavkov

#include <boat/catalogs.hpp>
#include <boat/commands.hpp>
#include "db.h"

std::unique_ptr<boat::db::catalog> make_catalog(
    std::string_view address,
    bool update)
{
    return boat::make_catalog(address, update);
}

std::unique_ptr<boat::db::catalog> make_gdal_catalog(
    char const* path,
    char const* driver,
    boat::db::raster const* raster)
{
    auto ret = std::make_unique<boat::gdal::catalog>();
    ret->dataset = raster ? boat::gdal::create(path, driver, *raster)
                          : boat::gdal::create(path, driver);
    return ret;
}

std::unique_ptr<boat::db::command> make_command(std::string const& address)
{
    return boat::make_command(address);
}
