// Andrew Naplavkov

#ifndef BOAT_GUI_DATASETS_DATASETS_HPP
#define BOAT_GUI_DATASETS_DATASETS_HPP

#if __has_include(<curl/curl.h>)
#include <boat/gui/datasets/slippy.hpp>
#endif
#include <boat/gui/datasets/sql.hpp>

namespace boat::gui::datasets {

inline std::shared_ptr<dataset> create(std::string_view uri,
                                       std::shared_ptr<caches::cache> cache)
{
    auto u = parse_uri(uri);
    check(!!u, "datasets::create");
    if (u->scheme == "slippy")
#if __has_include(<curl/curl.h>)
        return std::make_shared<slippy>(u->user, std::move(cache));
#else
        throw std::runtime_error(concat("datasets::create ", u->scheme));
#endif
    return std::make_shared<sql>(u->scheme, to_string(*u), std::move(cache));
}

}  // namespace boat::gui::datasets

#endif  // BOAT_GUI_DATASETS_DATASETS_HPP
