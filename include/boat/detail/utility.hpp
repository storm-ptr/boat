// Andrew Naplavkov

#ifndef BOAT_UTILITY_HPP
#define BOAT_UTILITY_HPP

#include <algorithm>
#include <bit>
#include <boost/math/constants/constants.hpp>
#include <charconv>
#include <concepts>
#include <cstddef>
#include <limits>
#include <locale>
#include <memory>
#include <numbers>
#include <ranges>
#include <span>
#include <sstream>
#include <stdexcept>
#include <type_traits>

namespace boat {

using boost::math::double_constants::degree;
using boost::math::double_constants::radian;
using std::numbers::phi;
using std::numbers::pi;
constexpr auto inv_phi = phi - 1;

template <class T>
concept arithmetic = std::is_arithmetic_v<T>;

template <class Lhs, class Rhs>
concept same_size = sizeof(Lhs) == sizeof(Rhs);

template <template <class...> class Tpl, class... Ts>
void specialization_test(Tpl<Ts...> const&);

template <class T, template <class...> class Tpl>
concept specialized = requires(T val) { specialization_test<Tpl>(val); };

template <class T>
concept ostream = specialized<T, std::basic_ostream>;

template <class R, class T>
concept range_of = std::same_as<std::ranges::range_value_t<R>, T>;

template <class T, auto del>
using unique_ptr = std::unique_ptr<T, decltype([](T* ptr) { del(ptr); })>;

template <class... Ts>
struct overloaded : Ts... {
    using Ts::operator()...;
};

constexpr auto as_bytes(auto* ptr)
{
    return reinterpret_cast<std::byte const*>(ptr);
}

constexpr auto as_chars(auto* ptr)
{
    return reinterpret_cast<char const*>(ptr);
}

constexpr auto single_span(arithmetic auto& val)
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

inline void check(bool success, char const* what)
{
    if (!success)
        throw std::runtime_error(what);
}

template <class CharT = char>
std::basic_string<CharT> concat(auto&&... vals)
{
    auto os = std::basic_ostringstream<CharT>{};
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
