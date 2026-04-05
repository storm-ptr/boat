// Andrew Naplavkov

#ifndef BOAT_DALS_HPP
#define BOAT_DALS_HPP

#include <boat/detail/uri.hpp>
#if __has_include(<gdal.h>)
#include <boat/gdal/dal.hpp>
#endif
#if __has_include(<curl/curl.h>)
#include <boat/slippy.hpp>
#endif
#include <boat/sql/commands.hpp>
#include <boat/sql/dal.hpp>

namespace boat {

inline std::unique_ptr<db::dal> make_dal(std::string_view url)
{
    constexpr auto gdal_prefix = std::string_view{"gdal://"};
    if (url.starts_with(gdal_prefix))
#if __has_include(<gdal.h>)
    {
        url.remove_prefix(gdal_prefix.size());
        auto ret = std::make_unique<gdal::dal>();
        ret->dataset = gdal::open(std::string{url}.data());
        return ret;
    }
#else
        throw std::runtime_error("compiled without gdal");
#endif
    if (url.starts_with("slippy"))
#if __has_include(<curl/curl.h>)
    {
        auto parsed = uri::parse(url);
        auto os = std::ostringstream{};
        if (parsed.scheme == "slippys")
            os << "https://";
        else
            os << "http://";
        os << parsed.host_spec << '/' << parsed.path;
        if (!parsed.query.empty())
            os << '?' << parsed.query;
        auto ret = std::make_unique<slippy::dal>();
        ret->user = parsed.user;
        ret->url = std::move(os).str();
        return ret;
    }
#else
        throw std::runtime_error("compiled without curl");
#endif
    auto ret = std::make_unique<sql::dal>();
    ret->command = sql::make_command(url);
    return ret;
}

}  // namespace boat

#endif  // BOAT_DALS_HPP
