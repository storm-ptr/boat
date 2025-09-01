// Andrew Naplavkov

#include <boat/blob.hpp>
#include <boost/test/unit_test.hpp>

BOOST_AUTO_TEST_CASE(blob_io)
{
    auto bytes = boat::blob{} << 7 << 3.14;
    int i;
    double d;
    boat::blob_view{bytes} >> i >> d;
    BOOST_CHECK_EQUAL(i, 7);
    BOOST_CHECK_EQUAL(d, 3.14);
}

BOOST_AUTO_TEST_CASE(blob_endian)
{
    auto bytes = boat::blob{} << char8_t(0xfe) << char8_t(0xff);
    for (auto [endian, expect] : {
             std::pair{std::endian::big, 0xfeff},
             std::pair{std::endian::little, 0xfffe},
         }) {
        auto in = boat::blob_view{bytes};
        BOOST_CHECK(boat::get<char16_t>(in, endian) == char16_t(expect));
    }
}
