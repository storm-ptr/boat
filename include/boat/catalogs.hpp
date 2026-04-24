// Andrew Naplavkov

#ifndef BOAT_CATALOGS_HPP
#define BOAT_CATALOGS_HPP

#include <boat/detail/uri.hpp>
#if __has_include(<gdal.h>)
#include <boat/gdal/catalog.hpp>
#endif
#if __has_include(<curl/curl.h>)
#include <boat/slippy.hpp>
#endif
#include <boat/sql/catalog.hpp>
#include <boat/sql/commands.hpp>

namespace boat {

inline std::unique_ptr<db::catalog> make_catalog(std::string_view url_or_gdal)
{
    if (any({"http://", "https://"}, prefix(url_or_gdal)))
#if __has_include(<curl/curl.h>)
    {
        auto parsed = uri::parse(url_or_gdal);
        auto os = std::ostringstream{};
        os << parsed.scheme << "://" << parsed.host_spec << '/' << parsed.path;
        if (!parsed.query.empty())
            os << '?' << parsed.query;
        auto ret = std::make_unique<slippy::catalog>();
        ret->user = parsed.user;
        ret->url = std::move(os).str();
        return ret;
    }
#else
        throw std::runtime_error("compiled without curl");
#endif
    if (sql::supported_url(url_or_gdal)) {
        auto ret = std::make_unique<sql::catalog>();
        ret->command = sql::make_command(url_or_gdal);
        return ret;
    }
#if __has_include(<gdal.h>)
    auto ret = std::make_unique<gdal::catalog>();
    ret->dataset = gdal::open(std::string{url_or_gdal}.data());
    return ret;
#else
    throw std::runtime_error("compiled without gdal");
#endif
}

}  // namespace boat

#endif  // BOAT_CATALOGS_HPP
