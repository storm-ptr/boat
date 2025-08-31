// Andrew Naplavkov

#ifndef BOAT_SQL_REFLECTION_HPP
#define BOAT_SQL_REFLECTION_HPP

#include <boat/blob.hpp>
#include <boat/geometry/detail/utility.hpp>
#include <boat/sql/vocabulary.hpp>
#include <boost/pfr.hpp>
#include <boost/type_index/ctti_type_index.hpp>
#include <chrono>

namespace boat::sql {
namespace detail {

template <std::integral>
constexpr auto type()
{
    return "bigint";
}

template <std::floating_point>
constexpr auto type()
{
    return "double precision";
}

template <std::same_as<std::string>>
constexpr auto type()
{
    return "varchar";
}

template <std::same_as<blob>>
constexpr auto type()
{
    return "blob";
}

template <geometry::ogc99>
constexpr auto type()
{
    return "geometry";
}

template <specialized<std::chrono::time_point>>
constexpr auto type()
{
    return "timestamp";
}

template <specialized<std::optional> T>
constexpr auto type()
{
    return type<typename T::value_type>();
}

template <class T>
constexpr auto types_as_array()
{
    auto ret = std::array<std::string_view, boost::pfr::tuple_size_v<T>>{};
    [&]<size_t... Is>(std::integer_sequence<size_t, Is...>) {
        ((ret[Is] = type<boost::pfr::tuple_element_t<Is, T>>()), ...);
    }(std::make_integer_sequence<size_t, boost::pfr::tuple_size_v<T>>{});
    return ret;
}

}  // namespace detail

template <class T>
table get_table()
{
    auto ret = table{.table_name{
        boost::typeindex::ctti_type_index::type_id<T>().pretty_name()}};
    if (auto pos = ret.table_name.find_last_of(' '); pos != std::string::npos)
        ret.table_name = ret.table_name.substr(pos + 1);
    for (auto [name, type] : std::views::zip(boost::pfr::names_as_array<T>(),
                                             detail::types_as_array<T>())) {
        auto col = column{.column_name{name}, .type_name{type}};
        if (type == detail::type<geometry::geographic::variant>())
            col.srid = col.epsg = 4326;
        ret.columns.push_back(std::move(col));
    }
    return ret;
}

}  // namespace boat::sql

#endif  // BOAT_SQL_REFLECTION_HPP
