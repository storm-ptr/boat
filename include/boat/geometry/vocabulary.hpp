// Andrew Naplavkov

#ifndef BOAT_GEOMETRY_VOCABULARY_HPP
#define BOAT_GEOMETRY_VOCABULARY_HPP

#include <boat/detail/utility.hpp>
#include <boost/geometry.hpp>
#include <boost/geometry/geometries/adapted/std_variant.hpp>
#include <boost/geometry/srs/epsg.hpp>
#include <boost/geometry/srs/transformation.hpp>
#include <map>

namespace boat::geometry {

namespace model = boost::geometry::model;
namespace srs = boost::geometry::srs;

template <class T>
using tag = boost::geometry::tag<T>::type;

template <class T>
struct has_tag : std::negation<std::is_same<tag<T>, void>> {};

template <class... Ts>
struct has_tag<std::variant<Ts...>> : std::conjunction<has_tag<Ts>...> {};

template <class T>
concept tagged = has_tag<T>::value;

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
concept single = std::derived_from<tag<T>, boost::geometry::single_tag>;

template <class T>
concept curve = single<T> && point<std::ranges::range_value_t<T>>;

template <class T>
concept projection_or_transformation =
    specialized<T, srs::projection> || specialized<T, srs::transformation>;

template <class T>
concept srs_params = std::constructible_from<srs::projection<>, T>;

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

    d2<CoordSys>::point operator()(geometry::point auto const& g) const
    {
        return {g.x(), g.y()};
    }

    d2<CoordSys>::box operator()(geometry::box auto const& g) const
    {
        return {(*this)(g.min_corner()), (*this)(g.max_corner())};
    }
};

using cartesian = d2<boost::geometry::cs::cartesian>;
using geographic = d2<boost::geometry::cs::geographic<boost::geometry::degree>>;
using matrix = boost::qvm::mat<double, 3, 3>;
using srs_variant = std::variant<srs::epsg, srs::proj4>;

template <class T>
using d2_of = d2<typename boost::geometry::coordinate_system<T>::type>;

template <class T>
concept ogc99 = tagged<T> && std::convertible_to<T, typename d2_of<T>::variant>;

template <class T>
constexpr auto variant_index_v = variant_index<typename d2_of<T>::variant, T>();

}  // namespace boat::geometry

template <class T>
    requires requires { typename T::boat_geometry_tag; }
struct boost::geometry::traits::tag<T> {
    using type = T::boat_geometry_tag;
};

#endif  // BOAT_GEOMETRY_VOCABULARY_HPP
