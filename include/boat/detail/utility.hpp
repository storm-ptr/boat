// Andrew Naplavkov

#ifndef BOAT_UTILITY_HPP
#define BOAT_UTILITY_HPP

#include <algorithm>
#include <bit>
#include <cmath>
#include <cstddef>
#include <ranges>
#include <span>
#include <sstream>
#include <stdexcept>
#include <variant>

namespace boat {

template <class T>
concept arithmetic = std::is_arithmetic_v<T>;

template <class T, class U>
concept same_size = sizeof(T) == sizeof(U);

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

constexpr auto single_span = [](arithmetic auto& v) {
    return std::span{&v, 1};
};

constexpr auto byteswap = overloaded{
    [](std::integral auto v) { return std::byteswap(v); },
    [](std::floating_point auto v) {
        std::ranges::reverse(std::as_writable_bytes(single_span(v)));
        return v;
    },
};

constexpr auto fraction = [](std::floating_point auto v) {
    return std::modf(v, &v);
};

constexpr auto mixed = [](std::endian e) {
    return std::endian::big != e && std::endian::little != e;
};

constexpr auto normal = [](std::integral auto v) { return !!v; };

void check(bool success, auto const& what)
    requires requires { std::runtime_error(what); }
{
    if (!success)
        throw std::runtime_error(what);
}

constexpr auto pow2 = []<std::integral T>(T exp) {
    check(exp >= 0, "pow2");
    return static_cast<T>(1uz << exp);
};

constexpr auto pow4 = [](std::integral auto exp) { return pow2(2 * exp); };

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
