// Andrew Naplavkov

#ifndef BOAT_DB_CATALOG_HPP
#define BOAT_DB_CATALOG_HPP

#include <boat/db/meta.hpp>
#include <boat/db/rowset.hpp>
#include <boat/tile.hpp>
#include <generator>

namespace boat::db {

struct catalog {
    virtual ~catalog() = default;

    virtual std::vector<source> sources() = 0;

    virtual std::vector<layer> layers() = 0;

    virtual table get_table(  //
        std::string_view schema_name,
        std::string_view table_name) = 0;

    virtual rowset select(table const&, page const&) = 0;

    virtual rowset select(table const&, bbox const&) = 0;

    virtual void insert(table const&, rowset const&) = 0;

    virtual table create(table const&) = 0;

    virtual void drop(  //
        std::string_view schema_name,
        std::string_view table_name) = 0;

    virtual raster get_raster(layer const&) = 0;

    virtual std::generator<std::pair<tile, blob>> mosaic(  //
        raster,
        std::vector<tile>) = 0;
};

}  // namespace boat::db

#endif  // BOAT_DB_CATALOG_HPP
