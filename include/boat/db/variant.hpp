// Andrew Naplavkov

#ifndef BOAT_DB_VARIANT_HPP
#define BOAT_DB_VARIANT_HPP

#include <boat/blob.hpp>
#include <cstdint>

namespace boat::db {

using null = std::monostate;
using variant_base = std::variant<null, int64_t, double, std::string, blob>;

struct variant : variant_base {
    using variant_base::variant_base;
    explicit variant(std::string_view v) { emplace<std::string>(v); }
    explicit operator bool() const { return has_value(); }
    bool has_value() const { return !std::holds_alternative<null>(*this); }
    void reset() { emplace<null>(); }
};

template <std::convertible_to<variant_base> T>
void read(variant const& in, T& out)
{
    auto vis = overloaded{
        [&](null) { out = {}; },
        [&](std::convertible_to<T> auto& v) { out = static_cast<T>(v); },
        [](...) { throw std::bad_variant_access{}; }};
    std::visit(vis, in);
}

void write(variant& out, std::convertible_to<variant_base> auto const& in)
{
    out = static_cast<variant>(in);
}

template <class T>
T get(variant const& var)
    requires requires(T val) { read(var, val); }
{
    T ret;
    read(var, ret);
    return ret;
}

variant to_variant(auto const& val)
    requires requires(variant var) { write(var, val); }
{
    auto ret = variant{};
    write(ret, val);
    return ret;
}

template <class>
struct kind;

template <std::integral T>
struct kind<T> {
    static constexpr auto value = "int";
};

template <std::floating_point T>
struct kind<T> {
    static constexpr auto value = "float";
};

template <>
struct kind<std::string> {
    static constexpr auto value = "varchar";
};

template <>
struct kind<blob> {
    static constexpr auto value = "blob";
};

}  // namespace boat::db

template <>
struct std::hash<boat::db::variant> {
    static size_t operator()(boat::db::variant const& that)
    {
        return std::hash<boat::db::variant_base>{}(that);
    }
};

#endif  // BOAT_DB_VARIANT_HPP
