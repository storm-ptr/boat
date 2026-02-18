// Andrew Naplavkov

#ifndef BOAT_SQL_REFLECTION_HPP
#define BOAT_SQL_REFLECTION_HPP

#include <boat/sql/detail/adaptors/adaptors.hpp>
#include <boost/type_index/ctti_type_index.hpp>

namespace boat::sql {

template <class T>
constexpr std::string_view type = adaptors::type_name<T>::value;

template <specialized<std::optional> T>
constexpr std::string_view type<T> = type<typename T::value_type>;

namespace detail {

template <class T>
constexpr auto types_as_array()
{
    auto ret = std::array<std::string_view, boost::pfr::tuple_size_v<T>>{};
    [&]<size_t... Is>(std::integer_sequence<size_t, Is...>) {
        ((ret[Is] = type<boost::pfr::tuple_element_t<Is, T>>), ...);
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
    for (auto [nm, tp] : std::views::zip(  //
             boost::pfr::names_as_array<T>(),
             detail::types_as_array<T>())) {
        auto col = column{.column_name{nm}, .lcase_type{tp}};
        if (tp == type<geometry::geographic::variant>)
            col.srid = col.epsg = 4326;
        ret.columns.push_back(std::move(col));
    }
    return ret;
}

inline table shrink_columns(table const& tbl)
{
    auto ret = tbl;
    std::erase_if(  //
        ret.columns,
        [&](auto& col) { return !adaptors::try_make(tbl.lcase_dbms, col); });
    for (auto& key : ret.index_keys)
        if (!std::ranges::contains(
                ret.columns, key.column_name, &column::column_name))
            key.column_name.clear();
    return ret;
}

inline std::vector<std::string> to_common_types(table const& tbl)
{
    auto ret = std::vector<std::string>{};
    for (auto& col : tbl.columns)
        ret.push_back(adaptors::make(tbl.lcase_dbms, col)->to_type({}).name);
    return ret;
}

}  // namespace boat::sql

#endif  // BOAT_SQL_REFLECTION_HPP
