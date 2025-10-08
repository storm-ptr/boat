// Andrew Naplavkov

#include <boost/test/unit_test.hpp>
#include <iostream>
#include "data.hpp"

BOOST_AUTO_TEST_CASE(pfr)
{
    auto objs = get_objects();
    auto rows = boat::pfr::to_rowset(objs);
    BOOST_CHECK(std::ranges::equal(objs,
                                   rows | boat::pfr::view<object_struct>,
                                   BOAT_LIFT(boost::pfr::eq_fields)));
    auto locale = std::locale{"en_US.UTF-8"};
    auto global_scope = revoke{&std::locale::global, locale};
    auto cout_scope = revoke{
        std::bind_front(&decltype(std::cout)::imbue, &std::cout), locale};
    std::cout << std::fixed << std::setprecision(2) << rows << std::endl;
}
