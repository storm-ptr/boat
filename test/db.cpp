// Andrew Naplavkov

#include <boost/test/unit_test.hpp>
#include "commands.hpp"
#include "data.hpp"

BOOST_AUTO_TEST_CASE(db_select)
{
    struct test {
        int64_t n;
        std::optional<double> d;
        std::string s;
    };
    auto expect = std::array{test{.s = "a"}, test{.n = 1, .d = 3.14, .s = "b"}};
    auto qry = boat::db::query{"select 0, null, 'a' union select 1, 3.14, 'b'"};
    for (auto cmd : commands())
        BOOST_CHECK(std::ranges::equal(expect,
                                       cmd->exec(qry) | boat::pfr::view<test>,
                                       BOAT_LIFT(boost::pfr::eq_fields)));
}

BOOST_AUTO_TEST_CASE(db_param)
{
    auto qry = boat::db::query{};
    auto sep1 = "\n select ";
    for (auto& row : boat::pfr::to_rowset(objects)) {
        qry << std::exchange(sep1, "\n union select ");
        auto sep2 = "";
        for (auto& var : row)
            qry << std::exchange(sep2, ", ") << var;
    }
    for (auto cmd : commands())
        BOOST_CHECK(
            std::ranges::equal(objects,
                               cmd->exec(qry) | boat::pfr::view<object_struct>,
                               BOAT_LIFT(boost::pfr::eq_fields)));
}
