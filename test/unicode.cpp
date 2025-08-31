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
    auto u32 = boat::unicode::string<char32_t>(u16);
    auto u8 = boat::unicode::string<char8_t>(u32);
    BOOST_CHECK(boat::unicode::string<char16_t>(u8) == u16);
    auto global = make_locale_scope(utf8_locale);
    auto wcout = make_locale_scope(utf8_locale, std::wcout);
    std::wcout << boat::unicode::string<wchar_t>(u8) << std::endl;
}
