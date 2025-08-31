// Andrew Naplavkov

#ifndef BOAT_SQL_DIALECTS_DIALECT_HPP
#define BOAT_SQL_DIALECTS_DIALECT_HPP

#include <boat/db/query.hpp>
#include <boat/sql/vocabulary.hpp>

namespace boat::sql::dialects {

struct dialect {
    virtual ~dialect() = default;

    virtual bool match(std::string_view dbms_name) const = 0;

    virtual db::query layers() const = 0;

    virtual db::query columns(std::string_view schema_name,
                              std::string_view table_name) const = 0;

    virtual db::query index_keys(std::string_view schema_name,
                                 std::string_view table_name) const = 0;

    virtual db::query select(table const&, page const&) const = 0;

    virtual db::query select(table const&, box const&) const = 0;

    virtual db::query schema() const = 0;

    virtual db::query srid(int epsg) const = 0;

    virtual db::query create(table const&) const = 0;
};

}  // namespace boat::sql::dialects

#endif  // BOAT_SQL_DIALECTS_DIALECT_HPP
