// Andrew Naplavkov

#ifndef BOAT_GUI_DATASETS_DATASETS_HPP
#define BOAT_GUI_DATASETS_DATASETS_HPP

#include <boat/detail/uri.hpp>
#include <boat/gui/caches/cache.hpp>
#if __has_include(<gdal.h>) && __has_include(<png.h>) && __has_include(<zlib.h>)
#include <boat/gui/datasets/gdal.hpp>
#endif
#if __has_include(<curl/curl.h>)
#include <boat/gui/datasets/slippy.hpp>
#endif
#include <boat/gui/datasets/sql.hpp>

namespace boat::gui::datasets {

inline std::shared_ptr<dataset> create(
    std::string_view url,
    std::shared_ptr<caches::cache> const& cache)
{
    if (url.starts_with("gdal"))
#if __has_include(<gdal.h>) && __has_include(<png.h>) && __has_include(<zlib.h>)
    {
        auto parsed = uri::parse(url);
        auto os = std::ostringstream{};
        os.imbue(std::locale::classic());
        if (parsed.host_spec.empty())
            os << parsed.path;
        else {
            if (parsed.scheme == "gdals")
                os << "/vsicurl/https://";
            else
                os << "/vsicurl/http://";
            os << parsed.host_spec << '/' << parsed.path;
        }
        return std::make_shared<gdal>(std::move(os).str(), cache);
    }
#else
        throw std::runtime_error("no gdal/png/zlib");
#endif
    if (url.starts_with("slippy"))
#if __has_include(<curl/curl.h>)
    {
        auto parsed = uri::parse(url);
        auto os = std::ostringstream{};
        os.imbue(std::locale::classic());
        if (parsed.scheme == "slippys")
            os << "https://";
        else
            os << "http://";
        os << parsed.host_spec << '/' << parsed.path;
        if (!parsed.query.empty())
            os << '?' << parsed.query;
        return std::make_shared<slippy>(
            std::string(parsed.user), std::move(os).str(), cache);
    }
#else
        throw std::runtime_error("no curl");
#endif
    return std::make_shared<sql>(std::string{url}, cache);
}

}  // namespace boat::gui::datasets

#endif  // BOAT_GUI_DATASETS_DATASETS_HPP
