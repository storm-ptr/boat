// Andrew Naplavkov

#ifndef BOAT_GUI_DATASETS_DATASETS_HPP
#define BOAT_GUI_DATASETS_DATASETS_HPP

#include <boat/gui/caches/cache.hpp>
#if __has_include(<curl/curl.h>)
#include <boat/gui/datasets/slippy.hpp>
#endif
#include <boat/gui/datasets/sql.hpp>

namespace boat::gui::datasets {

inline std::shared_ptr<dataset> create(
    std::string_view url,
    std::shared_ptr<caches::cache> const& cache)
{
    if (url.starts_with("slippy"))
#if __has_include(<curl/curl.h>)
        return std::make_shared<slippy>(std::string{uri::parse(url).user},
                                        cache);
#else
        throw std::runtime_error("no curl");
#endif
    return std::make_shared<sql>(std::string{url}, cache);
}

}  // namespace boat::gui::datasets

#endif  // BOAT_GUI_DATASETS_DATASETS_HPP
