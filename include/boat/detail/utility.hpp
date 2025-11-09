// Andrew Naplavkov

#ifndef BOAT_UTILITY_HPP
#define BOAT_UTILITY_HPP

#include <algorithm>
#include <bit>
#include <charconv>
#include <cstddef>
#include <limits>
#include <locale>
#include <ranges>
#include <span>
#include <sstream>
#include <stdexcept>

namespace boat {

template <class T>
concept arithmetic = std::is_arithmetic_v<T>;

template <class T, class U>
concept same_size = sizeof(T) == sizeof(U);

template <template <class...> class Tpl, class... Ts>
void specialization_test(Tpl<Ts...> const&);

template <class T, template <class...> class Tpl>
concept specialized = requires(T val) { specialization_test<Tpl>(val); };

template <class T>
concept ostream = specialized<T, std::basic_ostream>;

template <class T, class U>
concept range_of = std::same_as<std::ranges::range_value_t<T>, U>;

template <class T, auto del>
using unique_ptr = std::unique_ptr<T, decltype([](T* ptr) { del(ptr); })>;

template <class... Ts>
struct overloaded : Ts... {
    using Ts::operator()...;
};

auto as_bytes(auto* ptr)
{
    return reinterpret_cast<std::byte const*>(ptr);
}

auto as_chars(auto* ptr)
{
    return reinterpret_cast<char const*>(ptr);
}

auto single_span(arithmetic auto& val)
{
    return std::span{&val, 1};
}

auto byteswap(std::floating_point auto val)
{
    std::ranges::reverse(std::as_writable_bytes(single_span(val)));
    return val;
}

using std::byteswap;

constexpr bool mixed(std::endian e)
{
    return std::endian::big != e && std::endian::little != e;
}

void check(bool success, auto&& what)
    requires requires { std::runtime_error(what); }
{
    if (!success)
        throw std::runtime_error(what);
}

template <class T = char>
std::basic_string<T> concat(auto&&... vals)
{
    auto os = std::basic_ostringstream<T>{};
    os.imbue(std::locale::classic());
    ((os << vals), ...);
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
std::string to_chars(T val)
{
    auto ret = std::string(std::numeric_limits<T>::digits10 + 2, 0);
    auto [end, ec] = std::to_chars(ret.data(), ret.data() + ret.size(), val);
    check(ec == std::errc{}, "to_chars");
    ret.resize(end - ret.data());
    return ret;
}

}  // namespace boat

#endif  // BOAT_UTILITY_HPP
