// Andrew Naplavkov

#ifndef BOAT_DB_REFLECTION_HPP
#define BOAT_DB_REFLECTION_HPP

#include <boat/db/adapted/adapted.hpp>
#include <boat/db/meta.hpp>
#include <boost/pfr/core.hpp>
#include <boost/pfr/core_name.hpp>
#include <boost/type_index/ctti_type_index.hpp>

namespace boat::db {

template <class T>
table to_table()
{
    auto ret = table{.table_name{
        boost::typeindex::ctti_type_index::type_id<T>().pretty_name()}};
    ret.table_name.erase(0, ret.table_name.find_last_of(' ') + 1);
    [&]<size_t... Is>(std::index_sequence<Is...>) {
        ret.columns = {column{
            .kind{kind<boost::pfr::tuple_element_t<Is, T>>::value},
            .column_name{boost::pfr::get_name<Is, T>()}}...};
    }(std::make_index_sequence<boost::pfr::tuple_size_v<T>>{});
    for (auto& col : ret.columns)
        if (col.kind == kind<geometry::geographic::variant>::value)
            col.epsg = 4326;
    return ret;
}

}  // namespace boat::db

#endif  // BOAT_DB_REFLECTION_HPP
