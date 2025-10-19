// Andrew Naplavkov

#ifndef BOAT_GEOMETRY_UTILITY_HPP
#define BOAT_GEOMETRY_UTILITY_HPP

#include <boat/detail/utility.hpp>
#include <boat/geometry/model.hpp>
#include <boost/geometry/srs/transformation.hpp>
#include <cmath>

namespace boat::geometry {

constexpr auto root_geoid_area =
    2 * boost::math::double_constants::root_pi * 6'371'008.7714;

template <class T>
using as = d2<typename boost::geometry::coordinate_system<T>::type,
              typename boost::geometry::coordinate_type<T>::type>;

template <class T>
using tag = boost::geometry::tag<T>::type;

template <class T>
struct has_tag : std::negation<std::is_same<tag<T>, void>> {};

template <class... Ts>
struct has_tag<std::variant<Ts...>> : std::conjunction<has_tag<Ts>...> {};

template <class T>
concept box = std::same_as<tag<T>, boost::geometry::box_tag>;

template <class T>
concept dynamic = std::same_as<tag<T>, boost::geometry::dynamic_geometry_tag>;

template <class T>
concept linestring = std::same_as<tag<T>, boost::geometry::linestring_tag>;

template <class T>
concept multi = std::derived_from<tag<T>, boost::geometry::multi_tag>;

template <class T>
concept ogc99 =
    has_tag<T>::value && std::convertible_to<T, typename as<T>::variant>;

template <class T>
concept ogc99_or_box = ogc99<T> || box<T>;

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
    specialized<T, boost::geometry::srs::projection> ||
    specialized<T, boost::geometry::srs::transformation>;

template <class T>
concept srs_params =
    std::constructible_from<boost::geometry::srs::projection<>, T>;

template <class... Ts>
void variant_emplace(std::variant<Ts...>& var, size_t i)
{
    check(i < sizeof...(Ts), "variant_emplace");
    static std::variant<Ts...> const vars[] = {Ts{}...};
    var = vars[i];
}

template <class V, class T, size_t I = 0>
constexpr size_t variant_index()
{
    if constexpr (std::same_as<std::variant_alternative_t<I, V>, T>)
        return I;
    else
        return variant_index<V, T, I + 1>();
}

template <class T>
constexpr size_t variant_index_v = variant_index<typename as<T>::variant, T>();

auto frac(std::floating_point auto val)
{
    return std::modf(val, &val);
}

}  // namespace boat::geometry

#endif  // BOAT_GEOMETRY_UTILITY_HPP
