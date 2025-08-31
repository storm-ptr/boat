// Andrew Naplavkov

#include <boost/test/unit_test.hpp>
#include <iostream>
#include "data.hpp"

BOOST_AUTO_TEST_CASE(pfr)
{
    auto rows = boat::pfr::to_rowset(objects);
    BOOST_CHECK(std::ranges::equal(objects,
                                   rows | boat::pfr::view<object_struct>,
                                   BOAT_LIFT(boost::pfr::eq_fields)));
    auto global = make_locale_scope(utf8_locale);
    auto cout = make_locale_scope(utf8_locale, std::cout);
    std::cout << std::fixed << std::setprecision(2) << rows << std::endl;
}
