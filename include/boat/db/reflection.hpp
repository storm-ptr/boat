// Andrew Naplavkov

#ifndef BOAT_DB_REFLECTION_HPP
#define BOAT_DB_REFLECTION_HPP

#include <boat/db/adapted/adapted.hpp>
#include <boat/db/meta.hpp>
#include <boost/pfr.hpp>
#include <boost/type_index/ctti_type_index.hpp>

namespace boat::db {
namespace detail {

template <class T>
constexpr auto kinds_as_array()
{
    auto ret = std::array<std::string_view, boost::pfr::tuple_size_v<T>>{};
    [&]<size_t... Is>(std::integer_sequence<size_t, Is...>) {
        ((ret[Is] = kind<boost::pfr::tuple_element_t<Is, T>>::value), ...);
    }(std::make_integer_sequence<size_t, boost::pfr::tuple_size_v<T>>{});
    return ret;
}

}  // namespace detail

template <class T>
table to_table()
{
    auto ret = table{.table_name{
        boost::typeindex::ctti_type_index::type_id<T>().pretty_name()}};
    if (auto pos = ret.table_name.find_last_of(' '); pos != std::string::npos)
        ret.table_name = ret.table_name.substr(pos + 1);
    for (auto [k, n] : std::views::zip(  //
             detail::kinds_as_array<T>(),
             boost::pfr::names_as_array<T>())) {
        auto col = column{.kind{k}, .column_name{n}};
        if (col.kind == kind<geometry::geographic::variant>::value)
            col.epsg = 4326;
        ret.columns.push_back(std::move(col));
    }
    return ret;
}

}  // namespace boat::db

#endif  // BOAT_DB_REFLECTION_HPP
