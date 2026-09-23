// Andrew Naplavkov

#include <boat/db/query.hpp>
#include <boost/test/unit_test.hpp>
#include "data.hpp"

BOOST_AUTO_TEST_CASE(db)
{
    auto objs = get_objects();
    auto rs = boat::db::to_rowset(objs);
    BOOST_CHECK(std::ranges::equal(  //
        objs,
        rs | boat::db::view<udt>,
        BOAT_LIFT(boost::pfr::eq_fields)));
    auto locale = std::locale{"en_US.UTF-8"};
    auto global_scope = scoped_revoke{&std::locale::global, locale};
    auto cout_scope = scoped_revoke{
        std::bind_front(&decltype(std::cout)::imbue, &std::cout), locale};
    std::cout << std::fixed << std::setprecision(2) << rs << "\n";
}

BOOST_AUTO_TEST_CASE(db_query_identifier)
{
    auto qry = boat::db::query{boat::db::id{R"(a"b)"}};
    BOOST_CHECK_EQUAL(qry.text('"', "?"), R"("a""b")");
}

BOOST_AUTO_TEST_CASE(db_time_point_precision)
{
    using time_point = std::chrono::utc_time<std::chrono::microseconds>;
    for (auto str : {"2004-10-19 10:23:54",
                     "2004-10-19 10:23:54.12",
                     "2004-10-19 10:23:54.000249"}) {
        auto timestamp = boat::db::get<time_point>(str);
        auto var = boat::db::to_variant(timestamp);
        BOOST_CHECK_EQUAL(std::get<std::string>(var), str);
    }
}
