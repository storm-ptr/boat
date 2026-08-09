// Andrew Naplavkov

#ifndef BOAT_UTILITY_HPP
#define BOAT_UTILITY_HPP

#include <algorithm>
#include <bit>
#include <cmath>
#include <cstddef>
#include <execution>
#include <ranges>
#include <span>
#include <sstream>
#include <stdexcept>
#include <variant>

namespace boat {

template <class T>
concept arithmetic = std::is_arithmetic_v<T>;

template <class T>
concept execution_policy = std::is_execution_policy_v<T>;

template <template <class...> class Tpl, class... Ts>
void specialization_test(Tpl<Ts...> const&);

template <class T, template <class...> class Tpl>
concept specialized = requires(T v) { specialization_test<Tpl>(v); };

template <class T>
concept ostream = specialized<T, std::basic_ostream>;

template <class T, class U>
concept range_of = std::convertible_to<std::ranges::range_value_t<T>, U>;

template <class T, auto del>
using unique_ptr = std::unique_ptr<T, decltype([](T* ptr) { del(ptr); })>;

template <class... Ts>
struct overloaded : Ts... {
    using Ts::operator()...;
};

constexpr auto as_bytes = [](auto* ptr) {
    return reinterpret_cast<std::byte const*>(ptr);
};

constexpr auto as_chars = [](auto* ptr) {
    return reinterpret_cast<char const*>(ptr);
};

constexpr auto as_span = [](arithmetic auto& v) { return std::span{&v, 1}; };

constexpr auto byteswap = overloaded{
    [](std::integral auto v) { return std::byteswap(v); },
    [](std::floating_point auto v) {
        std::ranges::reverse(std::as_writable_bytes(as_span(v)));
        return v;
    },
};

constexpr auto frac = [](std::floating_point auto v) {
    return std::fmod(v, 1);
};

constexpr auto mixed = [](std::endian v) {
    return std::endian::big != v && std::endian::little != v;
};

template <class T>
void check(bool success, T&& what)
    requires std::constructible_from<std::string, T>
{
    if (!success)
        throw std::runtime_error(std::string{std::forward<T>(what)});
}

constexpr auto pow2 = []<std::integral T>(T exp) {
    check(exp >= 0 && exp < sizeof(T) * CHAR_BIT, "pow2");
    return static_cast<T>(1uz << exp);
};

template <std::floating_point T>
T wrap(T v, T lo, T hi)
{
    v = std::fmod(v - lo, hi - lo);
    return v + (v < 0 ? hi : lo);
}

template <std::floating_point T>
std::pair<T, bool> reflect(T v, T lo, T hi)
{
    auto two_hi = 2 * hi;
    v = wrap(v, lo, two_hi - lo);
    auto reflected = v > hi;
    return {reflected ? two_hi - v : v, reflected};
}

template <class... Ts>
void variant_emplace(std::variant<Ts...>& var, size_t index)
{
    check(index < sizeof...(Ts), "variant_emplace");
    static std::variant<Ts...> const vars[] = {Ts{}...};
    var = vars[index];
}

template <specialized<std::variant> V, class T, size_t I = 0>
constexpr size_t variant_index()
{
    if constexpr (std::same_as<std::variant_alternative_t<I, V>, T>)
        return I;
    else
        return variant_index<V, T, I + 1>();
}

}  // namespace boat

#endif  // BOAT_UTILITY_HPP
