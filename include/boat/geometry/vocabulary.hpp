// Andrew Naplavkov

#ifndef BOAT_GEOMETRY_VOCABULARY_HPP
#define BOAT_GEOMETRY_VOCABULARY_HPP

#include <boost/geometry.hpp>
#include <boost/geometry/geometries/adapted/std_variant.hpp>
#include <boost/pfr/ops_fields.hpp>
#include <map>

namespace boat::geometry {

namespace model = boost::geometry::model;

template <class CoordSys>
struct d2 {
    using point = model::d2::point_xy<double, CoordSys>;
    using linestring = model::linestring<point>;
    using polygon = model::polygon<point, false, true>;
    using multi_point = model::multi_point<point>;
    using multi_linestring = model::multi_linestring<linestring>;
    using multi_polygon = model::multi_polygon<polygon>;
    using box = model::box<point>;
    using segment = model::segment<point>;
    using grid = std::map<double, multi_point>;

    struct geometry_collection;

    using variant = std::variant<  //
        point,
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
};

using cartesian = d2<boost::geometry::cs::cartesian>;
using geographic = d2<boost::geometry::cs::geographic<boost::geometry::degree>>;
using matrix = boost::qvm::mat<double, 3, 3>;

struct tile {
    int z;
    int y;
    int x;

    static constexpr int size = 256;
    friend auto operator<=>(tile const&, tile const&) = default;
};

}  // namespace boat::geometry

template <class T>
    requires requires { typename T::boat_geometry_tag; }
struct boost::geometry::traits::tag<T> {
    using type = T::boat_geometry_tag;
};

template <>
struct std::hash<boat::geometry::tile> {
    static size_t operator()(boat::geometry::tile const& that)
    {
        return boost::pfr::hash_fields(that);
    }
};

#endif  // BOAT_GEOMETRY_VOCABULARY_HPP
