// Andrew Naplavkov

#ifndef BOAT_SQL_COMMANDS_HPP
#define BOAT_SQL_COMMANDS_HPP

#if __has_include(<libpq-fe.h>)
#include <boat/sql/libpq/command.hpp>
#else
#pragma message("no libpq")
#endif
#if __has_include(<mysql.h>)
#include <boat/sql/libmysql/command.hpp>
#else
#pragma message("no libmysql")
#endif
#if __has_include(<sql.h>)
#include <boat/sql/odbc/command.hpp>
#else
#pragma message("no odbc")
#endif
#if __has_include(<sqlite3.h>)
#include <boat/sql/sqlite/command.hpp>
#else
#pragma message("no sqlite")
#endif
#include <boat/detail/string.hpp>
#include <boat/detail/uri.hpp>

namespace boat::sql {

inline bool supported_url(std::string_view url)
{
    return any({"mysql://", "odbc://", "postgres://", "sqlite://"},
               prefix(url));
}

inline std::unique_ptr<db::command> make_command(std::string_view url)
{
    if (url.starts_with("mysql://"))
#if __has_include(<mysql.h>)
    {
        auto u = uri::parse(url);
        auto sock = socket_address::parse(u.host_spec);
        return std::make_unique<libmysql::command>(
            std::string{u.user}.data(),
            std::string{u.password}.data(),
            std::string{sock.host}.data(),
            sock.port.empty()
                ? 0
                : from_chars<int>(sock.port.data(), sock.port.size()),
            std::string{u.path}.data());
    }
#else
        throw std::runtime_error("compiled without libmysql");
#endif
    if (url.starts_with("odbc://"))
#if __has_include(<sql.h>)
    {
        auto u = uri::parse(url);
        auto sock = socket_address::parse(u.host_spec);
        auto os = std::ostringstream{};
        if (!u.user.empty())
            os << "uid=" << u.user << ';';
        if (!u.password.empty())
            os << "pwd=" << u.password << ';';
        if (!sock.host.empty())
            os << "server=" << sock.host << ';';
        if (!sock.port.empty())
            os << "port=" << sock.port << ';';
        if (!u.path.empty())
            os << "database=" << u.path << ';';
        constexpr auto replace_sep = [](char c) { return c == '&' ? ';' : c; };
        for (char c : u.query | std::views::transform(replace_sep))
            os << c;
        return std::make_unique<odbc::command>(std::move(os).str().data());
    }
#else
        throw std::runtime_error("compiled without odbc");
#endif
    if (url.starts_with("postgres://"))
#if __has_include(<libpq-fe.h>)
        return std::make_unique<libpq::command>(std::string{url}.data());
#else
        throw std::runtime_error("compiled without libpq");
#endif
    if (url.starts_with("sqlite://"))
#if __has_include(<sqlite3.h>)
        return std::make_unique<sqlite::command>(
            std::string{uri::parse(url).path}.data());
#else
        throw std::runtime_error("compiled without sqlite");
#endif
    throw std::runtime_error("sql::make_command");
}

}  // namespace boat::sql

#endif  // BOAT_SQL_COMMANDS_HPP
