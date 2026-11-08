// Andrew Naplavkov

#ifndef DB_H
#define DB_H

#include <boat/db/catalog.hpp>
#include <boat/db/command.hpp>
#include <memory>

std::unique_ptr<boat::db::catalog> make_catalog(
    std::string_view address,
    bool update = false);

std::unique_ptr<boat::db::catalog> make_gdal_catalog(
    char const* path,
    char const* driver,
    boat::db::raster const* raster = nullptr);

std::unique_ptr<boat::db::command> make_command(std::string const& address);

#endif  // DB_H
