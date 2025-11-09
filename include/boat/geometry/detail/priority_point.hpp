// Andrew Naplavkov

#ifndef BOAT_GEOMETRY_PRIORITY_POINT_HPP
#define BOAT_GEOMETRY_PRIORITY_POINT_HPP

#include <boat/geometry/vocabulary.hpp>

namespace boat::geometry {

struct priority_point {
    geographic::point point;
    double priority;
    size_t index;

    priority_point(geographic::point const& point,
                   geographic::point const& max,
                   size_t index)
        : point{point}
        , priority{-boost::geometry::comparable_distance(point, max)}
        , index{index}
    {
    }

    friend bool operator<(priority_point const& lhs, priority_point const& rhs)
    {
        return lhs.priority < rhs.priority;
    }
};

}  // namespace boat::geometry

#endif  // BOAT_GEOMETRY_PRIORITY_POINT_HPP
