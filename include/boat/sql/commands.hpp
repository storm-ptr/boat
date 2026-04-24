// Andrew Naplavkov

#ifndef BOAT_SQL_COMMANDS_HPP
#define BOAT_SQL_COMMANDS_HPP

#if __has_include(<libpq-fe.h>)
#include <boat/sql/libpq/command.hpp>
#endif
#if __has_include(<mysql.h>)
#include <boat/sql/libmysql/command.hpp>
#endif
#if __has_include(<sql.h>)
#include <boat/sql/odbc/command.hpp>
#endif
#if __has_include(<sqlite3.h>)
#include <boat/sql/sqlite/command.hpp>
#endif
#include <boat/detail/string.hpp>
#include <boat/detail/uri.hpp>

namespace boat::sql {

inline bool supported_url(std::string_view url)
{
    return any({"mysql://", "odbc://", "postgresql://", "sqlite://"},
               prefix(url));
}

inline std::unique_ptr<db::command> make_command(std::string_view url)
{
    if (url.starts_with("mysql://"))
#if __has_include(<mysql.h>)
    {
        auto parsed = uri::parse(url);
        auto socket = socket_address::parse(parsed.host_spec);
        return std::make_unique<libmysql::command>(
            std::string{parsed.user}.data(),
            std::string{parsed.password}.data(),
            std::string{socket.host}.data(),
            socket.port.empty()
                ? 0
                : from_chars<int>(socket.port.data(), socket.port.size()),
            std::string{parsed.path}.data());
    }
#else
        throw std::runtime_error("compiled without libmysql");
#endif
    if (url.starts_with("odbc://"))
#if __has_include(<sql.h>)
    {
        auto parsed = uri::parse(url);
        auto socket = socket_address::parse(parsed.host_spec);
        auto os = std::ostringstream{};
        if (!parsed.user.empty())
            os << "uid=" << parsed.user << ';';
        if (!parsed.password.empty())
            os << "pwd=" << parsed.password << ';';
        if (!socket.host.empty())
            os << "server=" << socket.host << ';';
        if (!socket.port.empty())
            os << "port=" << socket.port << ';';
        if (!parsed.path.empty())
            os << "database=" << parsed.path << ';';
        constexpr auto replace_sep = [](char c) { return c == '&' ? ';' : c; };
        for (char c : parsed.query | std::views::transform(replace_sep))
            os << c;
        return std::make_unique<odbc::command>(std::move(os).str().data());
    }
#else
        throw std::runtime_error("compiled without odbc");
#endif
    if (url.starts_with("postgresql://"))
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
