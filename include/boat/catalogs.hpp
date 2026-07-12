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

inline std::unique_ptr<db::catalog> make_catalog(std::string_view address)
{
    if (any({"http://", "https://"}, prefix(address)))
#if __has_include(<curl/curl.h>)
    {
        auto ret = std::make_unique<slippy::catalog>();
        auto u = uri::parse(address);
        ret->user = u.user;
        auto os = std::ostringstream{};
        os << u.scheme << "://" << u.host_spec << '/' << u.path;
        for (auto sep{"?"}; !u.query.empty();) {
            auto next = u.query.find('&');
            auto len = (next == u.query.npos) ? u.query.size() : next;
            auto pair = u.query.substr(0, len);
            u.query.remove_prefix(len + (next != u.query.npos));
            if (auto eq = pair.find('='); eq != pair.npos) {
                auto key = pair.substr(0, eq);
                auto val = pair.substr(eq + 1);
                if (key == "epsg") {
                    ret->epsg = from_chars<int>(val.data(), val.size());
                    continue;
                }
                if (key == "zmax") {
                    ret->zmax = from_chars<int>(val.data(), val.size());
                    continue;
                }
            }
            os << std::exchange(sep, "&") << pair;
        }
        ret->url = std::move(os).str();
        return ret;
    }
#else
        throw std::runtime_error("compiled without curl");
#endif
    if (sql::supported_url(address)) {
        auto ret = std::make_unique<sql::catalog>();
        ret->command = sql::make_command(address);
        return ret;
    }
#if __has_include(<gdal.h>)
    auto ret = std::make_unique<gdal::catalog>();
    ret->dataset = gdal::open(std::string{address}.data());
    return ret;
#else
    throw std::runtime_error("compiled without gdal");
#endif
}

}  // namespace boat

#endif  // BOAT_CATALOGS_HPP
