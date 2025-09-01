//< Andrew Naplavkov

#include <boat/gui/caches/lru.hpp>
#include <boost/test/unit_test.hpp>

BOOST_AUTO_TEST_CASE(caches_lru)
{
    auto lru = boat::gui::caches::lru{2u};
    lru.put(1, 1);                                         //< {1:1}
    lru.put(2, 2);                                         //< {1:1, 2:2}
    BOOST_CHECK_EQUAL(std::any_cast<int>(lru.get(1)), 1);  //< {2:2, 1:1}
    lru.put(3, 3);                                         //< {1:1, 3:3}
    BOOST_CHECK(not lru.get(2).has_value());               //< not found
    lru.put(4, 4);                                         //< {3:3, 4:4}
    BOOST_CHECK(not lru.get(1).has_value());               //< not found
    BOOST_CHECK_EQUAL(std::any_cast<int>(lru.get(3)), 3);  //< {4:4, 3:3}
    BOOST_CHECK_EQUAL(std::any_cast<int>(lru.get(4)), 4);  //< {3:3, 4:4}
}
