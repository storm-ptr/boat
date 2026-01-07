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
    constexpr auto gdal_prefix = std::string_view{"gdal://"};
    if (url.starts_with(gdal_prefix))
#if __has_include(<gdal.h>) && __has_include(<png.h>) && __has_include(<zlib.h>)
    {
        url.remove_prefix(gdal_prefix.size());
        return std::make_shared<gdal>(std::string{url}, cache);
    }
#else
        throw std::runtime_error("compiled without gdal/libpng/zlib");
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
        return std::make_shared<slippy>(
            std::string(parsed.user), std::move(os).str(), cache);
    }
#else
        throw std::runtime_error("compiled without curl");
#endif
    return std::make_shared<sql>(std::string{url}, cache);
}

}  // namespace boat::gui::datasets

#endif  // BOAT_GUI_DATASETS_DATASETS_HPP
