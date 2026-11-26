// Andrew Naplavkov

#ifndef DB_H
#define DB_H

#include <map>
#include <memory>
#include <string>

namespace boat::db {
struct catalog;
struct command;
struct raster;
}  // namespace boat::db

using catalog_map = std::map<std::string, std::unique_ptr<boat::db::catalog>>;

boat::db::catalog& get_catalog(catalog_map&, std::string const& address);

std::unique_ptr<boat::db::catalog> make_catalog(
    std::string_view address,
    bool update = false);

std::unique_ptr<boat::db::catalog> make_gdal_catalog(
    char const* path,
    char const* driver,
    boat::db::raster const* raster = nullptr);

std::unique_ptr<boat::db::command> make_command(std::string const& address);

#endif  // DB_H
