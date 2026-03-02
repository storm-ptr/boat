// Andrew Naplavkov

#ifndef BOAT_DB_ADAPTED_OPTIONAL_HPP
#define BOAT_DB_ADAPTED_OPTIONAL_HPP

#include <boat/db/variant.hpp>
#include <optional>

namespace boat::db {

template <class T>
void read(variant const& in, std::optional<T>& out)
    requires requires { read(in, out.emplace()); }
{
    in.has_value() ? read(in, out.emplace()) : out.reset();
}

template <class T>
void write(variant& out, std::optional<T> const& in)
    requires requires { write(out, *in); }
{
    in.has_value() ? write(out, *in) : out.reset();
}

template <specialized<std::optional> T>
struct kind<T> {
    static constexpr auto value = kind<typename T::value_type>::value;
};

}  // namespace boat::db

#endif  // BOAT_DB_ADAPTED_OPTIONAL_HPP
