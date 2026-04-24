// Andrew Naplavkov

#ifndef BOAT_TEST_DATA_HPP
#define BOAT_TEST_DATA_HPP

#include <boat/db/catalog.hpp>
#include <boat/db/io.hpp>
#include <boat/db/reflection.hpp>
#include <iostream>
#include "utility.hpp"

using utc_milliseconds = std::chrono::utc_time<std::chrono::milliseconds>;

struct udt {
    int id;
    utc_milliseconds time;
    boat::geometry::geographic::variant geom;
    std::string commander;
    std::optional<double> sample;
};

inline boat::db::table get_table()
{
    auto ret = boat::db::to_table<udt>();
    ret.index_keys = {
        {.index_name{"pk"},
         .column_name{boost::pfr::get_name<0, udt>()},
         .primary = true},
        {.index_name{"rtree"}, .column_name{boost::pfr::get_name<2, udt>()}}};
    return ret;
}

inline std::vector<udt> get_objects()
{
    return {
        {.id = 1,
         .time{boat::db::get<utc_milliseconds>("1961-04-12 06:07:01")},
         .geom{boost::geometry::from_wkt<boat::geometry::geographic::variant>(
             "GEOMETRYCOLLECTION(POINT (20 40),"
             "LINESTRING (40 40, 20 45, 45 30))")},
         .commander{
             boat::as_chars(u8"\u042e\u0440\u0438\u0439\u0020"
                            u8"\u0413\u0430\u0433\u0430\u0440\u0438\u043d")}},
        {.id = 2,
         .time{boat::db::get<utc_milliseconds>("1969-07-16 13:32:02.003")},
         .geom{boost::geometry::from_wkt<boat::geometry::geographic::variant>(
             "POLYGON((20 35,10 30,10 10,30 5,45 20,20 35),"
             "(30 20,20 15,20 25,30 20))")},
         .commander{"Neil Armstrong"},
         .sample = 21.55}};
}

inline void check(boat::db::catalog& cat)
{
    using namespace boat;
    static auto const objs = get_objects();
    static auto const page =
        db::page{.select_list = boost::pfr::names_as_array<udt>() |
                                std::ranges::to<std::vector<std::string>>(),
                 .limit = static_cast<int>(std::ranges::size(objs))};
    static auto const bbox = db::bbox{
        .select_list{std::string(boost::pfr::get_name<0, udt>())},
        .xmin = 9,
        .ymin = 9,
        .xmax = 11,
        .ymax = 11,
        .limit = int(std::ranges::size(objs)),
    };
    auto tbl = get_table();
    cat.drop(tbl.schema_name, tbl.table_name);
    tbl = cat.create(tbl);
    std::cout << tbl;
    cat.insert(tbl, db::to_rowset(objs));
    BOOST_CHECK(std::ranges::equal(  //
        objs,
        cat.select(tbl, page) | db::view<udt>,
        BOAT_LIFT(boost::pfr::eq_fields)));
    BOOST_CHECK(std::ranges::equal(  //
        std::array{2},
        cat.select(tbl, bbox) | db::view<int>));
}

#endif  // BOAT_TEST_DATA_HPP
