// Andrew Naplavkov

#ifndef BOAT_PFR_ADAPTED_OPTIONAL_HPP
#define BOAT_PFR_ADAPTED_OPTIONAL_HPP

#include <boat/pfr/variant.hpp>
#include <optional>

namespace boat::pfr {

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

}  // namespace boat::pfr

#endif  // BOAT_PFR_ADAPTED_OPTIONAL_HPP
