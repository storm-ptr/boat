// Andrew Naplavkov

#ifndef BOAT_GUI_DESCRIBE_HPP
#define BOAT_GUI_DESCRIBE_HPP

#include <boat/db/catalog.hpp>
#include <boat/gui/caches/cache.hpp>

namespace boat::gui {

using catalog_getter = std::move_only_function<db::catalog&() const>;

inline db::table get_table(
    caches::cache* cache,
    size_t cache_key,
    db::layer const& lyr,
    catalog_getter const& cat)
{
    return get_or_invoke(cache, cache_key, {}, [&] {
        return cat().get_table(lyr.schema_name, lyr.table_name);
    });
}

inline db::raster get_raster(
    caches::cache* cache,
    size_t cache_key,
    db::layer const& lyr,
    catalog_getter const& cat)
{
    return get_or_invoke(
        cache, cache_key, {}, [&] { return cat().get_raster(lyr); });
}

}  // namespace boat::gui

#endif  // BOAT_GUI_DESCRIBE_HPP
