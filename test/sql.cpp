// Andrew Naplavkov

#include <boat/sql/api.hpp>
#include <boat/sql/io.hpp>
#include <boost/test/unit_test.hpp>
#include <iostream>
#include "commands.hpp"
#include "data.hpp"

BOOST_AUTO_TEST_CASE(sql_api)
{
    using namespace boat;
    auto page =
        sql::page{.select_list = boost::pfr::names_as_array<object_struct>() |
                                 std::ranges::to<std::vector<std::string>>(),
                  .limit = int(std::ranges::size(objects))};
    auto box = sql::box{
        .select_list = {std::string(boost::pfr::get_name<0, object_struct>())},
        .xmin = 9,
        .ymin = 9,
        .xmax = 11,
        .ymax = 11,
        .limit = int(std::ranges::size(objects))};
    for (auto cmd : commands()) {
        cmd->set_autocommit(false);
        cmd->exec({"drop table if exists ", db::id{object_table.table_name}});
        auto tbl = sql::create(*cmd, object_table);
        std::cout << tbl;
        sql::insert(*cmd, tbl, pfr::to_rowset(objects));
        BOOST_CHECK(std::ranges::equal(
            objects,
            sql::select(*cmd, tbl, page) | pfr::view<object_struct>,
            BOAT_LIFT(boost::pfr::eq_fields)));
        BOOST_CHECK(std::ranges::equal(
            std::array{2}, sql::select(*cmd, tbl, box) | pfr::view<int>));
    }
}
