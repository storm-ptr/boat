// Andrew Naplavkov

#ifndef BOAT_DB_COMMANDS_HPP
#define BOAT_DB_COMMANDS_HPP

#if __has_include(<libpq-fe.h>)
#include <boat/db/libpq/command.hpp>
#endif
#if __has_include(<sql.h>)
#include <boat/db/odbc/command.hpp>
#endif
#if __has_include(<sqlite3.h>)
#include <boat/db/sqlite/command.hpp>
#endif
#include <boat/detail/uri.hpp>

namespace boat::db {

inline std::unique_ptr<command> create(char const* scheme,
                                       char const* connection)
{
#if __has_include(<sql.h>)
    if (!std::strcmp("odbc", scheme))
        return std::make_unique<odbc::command>(connection);
#endif
#if __has_include(<libpq-fe.h>)
    if (any({"postgres", "postgresql"}, equal(scheme)))
        return std::make_unique<libpq::command>(connection);
#endif
#if __has_include(<sqlite3.h>)
    if (!std::strcmp("sqlite", scheme))
        return std::make_unique<sqlite::command>(connection);
#endif
    throw std::runtime_error(concat("db::create ", scheme));
}

inline std::unique_ptr<command> create(std::string const& scheme,
                                       std::string const& connection)
{
    return create(scheme.data(), connection.data());
}

inline std::unique_ptr<command> create(std::string_view uri)
{
    auto u = parse_uri(uri);
    check(!!u, "db::create");
    return create(u->scheme, to_string(*u));
}

}  // namespace boat::db

#endif  // BOAT_DB_COMMANDS_HPP
