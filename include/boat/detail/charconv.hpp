// Andrew Naplavkov

#ifndef BOAT_CHARCONV_HPP
#define BOAT_CHARCONV_HPP

#include <boat/detail/utility.hpp>
#include <charconv>
#include <limits>
#include <locale>

namespace boat {

template <class T = char>
std::basic_string<T> concat(auto&&... vs)
{
    auto os = std::basic_ostringstream<T>{};
    os.imbue(std::locale::classic());
    ((os << vs), ...);
    return std::move(os).str();
}

template <arithmetic T>
T from_chars(char const* str, size_t len)
{
    T ret;
    auto [end, ec] = std::from_chars(str, str + len, ret);
    check(ec == std::errc{} && end == str + len, "from_chars");
    return ret;
}

template <std::integral T>
std::string to_chars(T v)
{
    auto ret = std::string(std::numeric_limits<T>::digits10 + 2, 0);
    auto [end, ec] = std::to_chars(ret.data(), ret.data() + ret.size(), v);
    check(ec == std::errc{}, "to_chars");
    ret.resize(end - ret.data());
    return ret;
}

inline std::string to_lower(std::string_view str)
{
    auto loc = std::locale::classic();
    auto fn = [&](char ch) -> char { return std::tolower(ch, loc); };
    return str | std::views::transform(fn) | std::ranges::to<std::string>();
}

}  // namespace boat

#endif  // BOAT_CHARCONV_HPP
