// Andrew Naplavkov

#ifndef BOAT_PFR_ADAPTED_TIME_POINT_HPP
#define BOAT_PFR_ADAPTED_TIME_POINT_HPP

#include <boat/pfr/variant.hpp>
#include <chrono>
#include <format>

namespace boat::pfr {

template <class... Ts>
void read(variant const& in, std::chrono::time_point<Ts...>& out)
{
    if (in.has_value())
        std::istringstream{std::get<std::string>(in)} >>
            std::chrono::parse("%F %T", out);
    else
        out = {};
}

template <class... Ts>
void write(variant& out, std::chrono::time_point<Ts...> in)
{
    out = concat(std::format("{:L%F %T}", in));
}

}  // namespace boat::pfr

#endif  // BOAT_PFR_ADAPTED_TIME_POINT_HPP
