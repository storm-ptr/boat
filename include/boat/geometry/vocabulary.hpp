// Andrew Naplavkov

#ifndef BOAT_GEOMETRY_VOCABULARY_HPP
#define BOAT_GEOMETRY_VOCABULARY_HPP

#include <boat/detail/utility.hpp>
#include <boost/geometry/geometries/adapted/std_variant.hpp>
#include <boost/geometry/geometries/geometries.hpp>
#include <boost/qvm/mat.hpp>
#include <map>
#include <tuple>

namespace boat::geometry {

namespace cs = boost::geometry::cs;
namespace model = boost::geometry::model;

template <class T>
using tag = boost::geometry::tag_t<T>;

template <class T>
constexpr bool has_tag = !std::is_same_v<tag<T>, void>;

template <class... Ts>
constexpr bool has_tag<std::variant<Ts...>> = (has_tag<Ts> && ...);

template <class T>
concept tagged = has_tag<std::remove_cvref_t<T>>;

template <class T>
concept box = std::same_as<tag<T>, boost::geometry::box_tag>;

template <class T>
concept dynamic = std::same_as<tag<T>, boost::geometry::dynamic_geometry_tag>;

template <class T>
concept linestring = std::same_as<tag<T>, boost::geometry::linestring_tag>;

template <class T>
concept multi = std::derived_from<tag<T>, boost::geometry::multi_tag>;

template <class T>
concept multi_point = std::same_as<tag<T>, boost::geometry::multi_point_tag>;

template <class T>
concept point = std::same_as<tag<T>, boost::geometry::point_tag>;

template <class T>
concept polygon = std::same_as<tag<T>, boost::geometry::polygon_tag>;

template <class T, class U>
concept same_tag = std::same_as<tag<T>, tag<U>>;

template <class T>
concept segment = std::same_as<tag<T>, boost::geometry::segment_tag>;

template <class T>
concept single = std::derived_from<tag<T>, boost::geometry::single_tag>;

template <class T>
concept curve = single<T> && point<std::ranges::range_value_t<T>>;

template <class Coord, class CoordSys>
struct d2 {
    using point = model::d2::point_xy<Coord, CoordSys>;
    using linestring = model::linestring<point>;
    using polygon = model::polygon<point, false, true>;
    using multi_point = model::multi_point<point>;
    using multi_linestring = model::multi_linestring<linestring>;
    using multi_polygon = model::multi_polygon<polygon>;
    using box = model::box<point>;
    using segment = model::segment<point>;
    using grid = std::map<Coord, multi_point>;

    struct geometry_collection;

    using variant = std::variant<
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

    static point operator()(geometry::point auto const& v)
    {
        return {Coord(v.x()), Coord(v.y())};
    }

    static box operator()(geometry::box auto const& v)
    {
        return {operator()(v.min_corner()), operator()(v.max_corner())};
    }
};

using cartesian = d2<double, cs::cartesian>;
using geographic = d2<double, cs::geographic<boost::geometry::degree>>;
using matrix = boost::qvm::mat<double, 3, 3>;
using raster = d2<int, cs::cartesian>;

template <class T>
using d2_t =
    d2<boost::geometry::coordinate_type_t<T>,
       boost::geometry::coordinate_system_t<T>>;

template <class T>
concept ogc =
    tagged<T> && std::same_as<boost::geometry::coordinate_type_t<T>, double> &&
    std::convertible_to<T, typename d2_t<T>::variant>;

template <class T>
constexpr auto variant_index_v = variant_index<typename d2_t<T>::variant, T>();

constexpr auto xywh = [](box auto const& v) {
    auto a = v.min_corner(), b = v.max_corner();
    return std::tuple{a.x(), a.y(), b.x() - a.x(), b.y() - a.y()};
};

}  // namespace boat::geometry

template <class T>
    requires requires { typename T::boat_geometry_tag; }
struct boost::geometry::traits::tag<T> {
    using type = T::boat_geometry_tag;
};

#endif  // BOAT_GEOMETRY_VOCABULARY_HPP
