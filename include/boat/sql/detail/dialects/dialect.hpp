// Andrew Naplavkov

#ifndef BOAT_SQL_DIALECTS_DIALECT_HPP
#define BOAT_SQL_DIALECTS_DIALECT_HPP

#include <boat/db/meta.hpp>
#include <boat/db/query.hpp>

namespace boat::sql::dialects {

struct dialect {
    virtual ~dialect() = default;

    virtual db::query layers() const = 0;

    virtual db::query columns(std::string_view schema_name,
                              std::string_view table_name) const = 0;

    virtual db::query index_keys(std::string_view schema_name,
                                 std::string_view table_name) const = 0;

    virtual db::query select(db::table const&, db::page const&) const = 0;

    virtual db::query select(db::table const&, db::bbox const&) const = 0;

    virtual db::query schema() const = 0;

    virtual db::query srid(int epsg) const = 0;

    virtual db::query create(db::table const&) const = 0;
};

}  // namespace boat::sql::dialects

#endif  // BOAT_SQL_DIALECTS_DIALECT_HPP
