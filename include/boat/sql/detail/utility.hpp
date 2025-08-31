// Andrew Naplavkov

#ifndef BOAT_SQL_UTILITY_HPP
#define BOAT_SQL_UTILITY_HPP

#include <boat/detail/utility.hpp>
#include <boat/sql/vocabulary.hpp>

namespace boat::sql {

constexpr auto geo = [](column const& col) { return col.srid > 0; };

constexpr auto constructible = [](range_of<index_key> auto&& idx) {
    return std::ranges::all_of(idx, [](index_key const& key) {
        return !key.partial && !key.column_name.empty();
    });
};

constexpr auto orderable = [](range_of<index_key> auto&& idx) {
    return std::ranges::all_of(idx, [](index_key const& key) {
        return !key.partial && !key.column_name.empty() && key.unique;
    });
};

constexpr auto primary = [](range_of<index_key> auto&& idx) {
    return std::ranges::all_of(
        idx, [](index_key const& key) { return key.primary; });
};

bool any_geo(range_of<column> auto&& cols, std::string_view column_name)
{
    return std::ranges::any_of(cols, [=](column const& col) {
        return geo(col) && col.column_name == column_name;
    });
}

auto find_or_geo(range_of<column> auto&& cols, std::string_view column_name)
{
    return std::ranges::find_if(cols, [=](column const& col) {
        return column_name.empty() ? geo(col) : col.column_name == column_name;
    });
}

bool any_primary(range_of<index_key> auto&& keys, std::string_view column_name)
{
    return std::ranges::any_of(keys, [=](index_key const& key) {
        return key.primary && key.column_name == column_name;
    });
}

}  // namespace boat::sql

#endif  // BOAT_SQL_UTILITY_HPP
