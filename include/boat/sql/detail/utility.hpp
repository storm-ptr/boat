// Andrew Naplavkov

#ifndef BOAT_SQL_UTILITY_HPP
#define BOAT_SQL_UTILITY_HPP

#include <boat/db/meta.hpp>
#include <boat/detail/utility.hpp>

namespace boat::sql {

bool any(std::initializer_list<std::string_view> list, auto&& pred)
{
    return std::ranges::any_of(list, pred);
}

constexpr auto same(std::string_view lhs)
{
    return [lhs](std::string_view rhs) { return lhs == rhs; };
}

constexpr auto in(std::string_view lhs)
{
    return [lhs](std::string_view rhs) { return lhs.contains(rhs); };
}

constexpr auto has(std::string_view lhs)
{
    return [lhs](std::string_view rhs) { return rhs.contains(lhs); };
}

constexpr auto is_mssql = has("microsoft sql server");
constexpr auto is_mysql = has("mysql");
constexpr auto is_postgres = has("postgres");
constexpr auto is_sqlite = has("sqlite");

constexpr auto geo = overloaded{
    [](db::column const& col) { return col.epsg > 0; },
    [](range_of<db::column> auto&& cols, std::string_view column_name) {
        return std::ranges::any_of(cols, [=](auto& col) {
            return col.epsg > 0 && col.column_name == column_name;
        });
    }};

db::column const& find(  //
    range_of<db::column> auto&& cols,
    std::string_view column_name)

{
    auto it = std::ranges::find(cols, column_name, &db::column::column_name);
    check(it != cols.end(), column_name);
    return *it;
}

db::column const& find_geo(  //
    range_of<db::column> auto&& cols,
    std::string_view column_name)
{
    auto it = std::ranges::find_if(cols, [=](auto& col) {
        return geo(col) &&
               (column_name.empty() || col.column_name == column_name);
    });
    check(it != cols.end(), column_name);
    return *it;
}

constexpr auto constructible = [](range_of<db::index_key> auto&& idx) {
    return std::ranges::all_of(idx, [](auto& key) {
        return !key.partial && !key.column_name.empty();
    });
};

constexpr auto orderable = [](range_of<db::index_key> auto&& idx) {
    return std::ranges::all_of(idx, [](auto& key) {
        return !key.partial && !key.column_name.empty() && key.unique;
    });
};

constexpr auto primary = overloaded{
    [](range_of<db::index_key> auto&& idx) {
        return std::ranges::all_of(idx, [](auto& key) { return key.primary; });
    },
    [](range_of<db::index_key> auto&& keys, std::string_view column_name) {
        return std::ranges::any_of(keys, [=](auto& key) {
            return key.primary && key.column_name == column_name;
        });
    }};

}  // namespace boat::sql

#endif  // BOAT_SQL_UTILITY_HPP
