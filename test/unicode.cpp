// Andrew Naplavkov

#include <boat/detail/unicode.hpp>
#include <boost/test/unit_test.hpp>
#include <iostream>
#include "utility.hpp"

BOOST_AUTO_TEST_CASE(unicode)
{
    auto u16 = std::u16string{
        0x68,
        0x65,
        0x6c,
        0x6c,
        0x6f,
        0x20,
        0x43f,
        0x440,
        0x438,
        0x432,
        0x435,
        0x442,
    };
    auto u32 = boat::unicode::utf32(u16);
    auto u8 = boat::unicode::utf8(u32);
    BOOST_CHECK(boat::unicode::utf16(u8) == u16);
    auto locale = std::locale{"en_US.UTF-8"};
    auto global_scope = revoke{&std::locale::global, locale};
    auto wcout_scope = revoke{
        std::bind_front(&decltype(std::wcout)::imbue, &std::wcout), locale};
    std::wcout << boat::unicode::utf<wchar_t>(u8) << std::endl;
}
