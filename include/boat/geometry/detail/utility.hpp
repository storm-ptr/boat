// Andrew Naplavkov

#ifndef BOAT_GEOMETRY_UTILITY_HPP
#define BOAT_GEOMETRY_UTILITY_HPP

#include <boat/geometry/concepts.hpp>

namespace boat::geometry {

auto add_value(point auto p, double d)
{
    boost::geometry::add_value(p, d);
    return p;
}

template <point T>
constexpr auto cast = [](point auto const& p) {
    T ret;
    boost::geometry::assign_point(ret, p);
    return ret;
};

}  // namespace boat::geometry

#endif  // BOAT_GEOMETRY_UTILITY_HPP
