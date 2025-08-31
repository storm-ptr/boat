// Andrew Naplavkov

#ifndef BOAT_GEOMETRY_MODEL_HPP
#define BOAT_GEOMETRY_MODEL_HPP

#include <boost/geometry.hpp>
#include <boost/geometry/geometries/adapted/std_variant.hpp>
#include <map>

namespace boat::geometry {

namespace model = boost::geometry::model;

template <class CoordinateSystem, class Distance = double>
struct d2 {
    using point = model::d2::point_xy<Distance, CoordinateSystem>;
    using linestring = model::linestring<point>;
    using polygon = model::polygon<point, false, true>;
    using multi_point = model::multi_point<point>;
    using multi_linestring = model::multi_linestring<linestring>;
    using multi_polygon = model::multi_polygon<polygon>;

    struct geometry_collection;

    using variant = std::variant<point,
                                 linestring,
                                 polygon,
                                 multi_point,
                                 multi_linestring,
                                 multi_polygon,
                                 geometry_collection>;

    struct geometry_collection : model::geometry_collection<variant> {
        using model::geometry_collection<variant>::geometry_collection;
        using boat_geometry_tag = boost::geometry::geometry_collection_tag;
    };

    using box = model::box<point>;
    using grid = std::map<Distance, multi_point>;
};

using cartesian = d2<boost::geometry::cs::cartesian>;
using geographic = d2<boost::geometry::cs::geographic<boost::geometry::degree>>;

}  // namespace boat::geometry

template <class T>
    requires requires { typename T::boat_geometry_tag; }
struct boost::geometry::traits::tag<T> {
    using type = T::boat_geometry_tag;
};

#endif  // BOAT_GEOMETRY_MODEL_HPP
