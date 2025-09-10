// Andrew Naplavkov

#ifndef BOAT_DB_SQLITE_UTILITY_HPP
#define BOAT_DB_SQLITE_UTILITY_HPP

#include <sqlite3.h>
// strictly ordered
#include <spatialite.h>
#include <boat/pfr/variant.hpp>

namespace boat::db::sqlite {

void check(int rc, auto& dbc)
    requires requires { sqlite3_errmsg(dbc.get()); }
{
    switch (rc) {
        case SQLITE_DONE:
        case SQLITE_OK:
        case SQLITE_ROW:
            return;
    }
    throw std::runtime_error(dbc ? sqlite3_errmsg(dbc.get()) : "sqlite");
}

inline int bind_value(sqlite3_stmt* stmt, int i, pfr::variant const& var)
{
    auto vis =
        overloaded{[=](pfr::null) { return sqlite3_bind_null(stmt, i); },
                   [=](int64_t v) { return sqlite3_bind_int64(stmt, i, v); },
                   [=](double v) { return sqlite3_bind_double(stmt, i, v); },
                   [=](std::string_view v) {
                       return sqlite3_bind_text(
                           stmt, i, v.data(), int(v.size()), SQLITE_STATIC);
                   },
                   [=](blob_view v) {
                       return sqlite3_bind_blob(
                           stmt, i, v.data(), int(v.size()), SQLITE_STATIC);
                   }};
    return std::visit(vis, var);
}

inline pfr::variant column_value(sqlite3_stmt* stmt, int col)
{
    auto type = sqlite3_column_type(stmt, col);
    switch (type) {
        case SQLITE_NULL:
            return {};
        case SQLITE_INTEGER:
            return sqlite3_column_int64(stmt, col);
        case SQLITE_FLOAT:
            return sqlite3_column_double(stmt, col);
        case SQLITE_TEXT:
            return pfr::variant{std::in_place_type<std::string>,
                                as_chars(sqlite3_column_text(stmt, col)),
                                sqlite3_column_bytes(stmt, col)};
        case SQLITE_BLOB:
            return pfr::variant{std::in_place_type<blob>,
                                as_bytes(sqlite3_column_blob(stmt, col)),
                                sqlite3_column_bytes(stmt, col)};
    }
    throw std::runtime_error{concat(sqlite3_column_name(stmt, col), " ", type)};
}

}  // namespace boat::db::sqlite

#endif  // BOAT_DB_SQLITE_UTILITY_HPP
