// Andrew Naplavkov

#ifndef BOAT_DB_COMMANDS_HPP
#define BOAT_DB_COMMANDS_HPP

#if __has_include(<libpq-fe.h>)
#include <boat/db/libpq/command.hpp>
#endif
#if __has_include(<mysql.h>)
#include <boat/db/libmysql/command.hpp>
#endif
#if __has_include(<sql.h>)
#include <boat/db/odbc/command.hpp>
#endif
#if __has_include(<sqlite3.h>)
#include <boat/db/sqlite/command.hpp>
#endif
#include <boat/detail/uri.hpp>

namespace boat::db {

inline std::unique_ptr<command> create(std::string_view url)
{
    if (url.starts_with("mysql"))
#if __has_include(<mysql.h>)
    {
        auto parsed = uri::parse(url);
        auto adr = socket_address::parse(parsed.host_spec);
        return std::make_unique<libmysql::command>(
            std::string{parsed.user}.data(),
            std::string{parsed.password}.data(),
            std::string{adr.host}.data(),
            adr.port.empty()
                ? 0
                : from_chars<int>(adr.port.data(), adr.port.size()),
            std::string{parsed.path}.data());
    }
#else
        throw std::runtime_error("no libmysql");
#endif
    if (url.starts_with("odbc"))
#if __has_include(<sql.h>)
    {
        auto parsed = uri::parse(url);
        auto adr = socket_address::parse(parsed.host_spec);
        auto os = std::ostringstream{};
        os.imbue(std::locale::classic());
        if (!parsed.user.empty())
            os << "uid=" << parsed.user << ';';
        if (!parsed.password.empty())
            os << "pwd=" << parsed.password << ';';
        if (!adr.host.empty())
            os << "server=" << adr.host << ';';
        if (!adr.port.empty())
            os << "port=" << adr.port << ';';
        if (!parsed.path.empty())
            os << "database=" << parsed.path << ';';
        auto replace_sep = [](char c) { return c == '&' ? ';' : c; };
        for (char c : parsed.query | std::views::transform(replace_sep))
            os << c;
        return std::make_unique<odbc::command>(std::move(os).str().data());
    }
#else
        throw std::runtime_error("no odbc");
#endif
    if (url.starts_with("postgres"))
#if __has_include(<libpq-fe.h>)
        return std::make_unique<libpq::command>(std::string{url}.data());
#else
        throw std::runtime_error("no libpq");
#endif
    if (url.starts_with("sqlite"))
#if __has_include(<sqlite3.h>)
        return std::make_unique<sqlite::command>(
            std::string{uri::parse(url).path}.data());
#else
        throw std::runtime_error("no sqlite");
#endif
    throw std::runtime_error("db::create");
}

}  // namespace boat::db

#endif  // BOAT_DB_COMMANDS_HPP
