// Andrew Naplavkov

#ifndef BOAT_STRING_HPP
#define BOAT_STRING_HPP

#include <boat/detail/utility.hpp>
#include <charconv>
#include <limits>
#include <locale>

namespace boat {

bool any(std::initializer_list<std::string_view> list, auto&& pred)
{
    return std::ranges::any_of(list, pred);
}

constexpr auto same(std::string_view lhs)
{
    return [lhs](std::string_view rhs) { return lhs == rhs; };
}

constexpr auto in(std::string_view lhs)
{
    return [lhs](std::string_view rhs) { return lhs.contains(rhs); };
}

constexpr auto has(std::string_view lhs)
{
    return [lhs](std::string_view rhs) { return rhs.contains(lhs); };
}

constexpr auto prefix(std::string_view lhs)
{
    return [lhs](std::string_view rhs) { return lhs.starts_with(rhs); };
}

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

constexpr auto to_lower = [](std::string_view str) {
    auto loc = std::locale::classic();
    auto fn = [&](char ch) -> char { return std::tolower(ch, loc); };
    return str | std::views::transform(fn) | std::ranges::to<std::string>();
};

}  // namespace boat

#endif  // BOAT_STRING_HPP
