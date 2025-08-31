// Andrew Naplavkov

#include <boost/test/unit_test.hpp>
#include "commands.hpp"
#include "data.hpp"

BOOST_AUTO_TEST_CASE(db)
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
