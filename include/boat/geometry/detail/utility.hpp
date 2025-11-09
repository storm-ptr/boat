// Andrew Naplavkov

#ifndef BOAT_GEOMETRY_UTILITY_HPP
#define BOAT_GEOMETRY_UTILITY_HPP

#include <boat/detail/utility.hpp>
#include <boat/geometry/vocabulary.hpp>
#include <boost/geometry/srs/transformation.hpp>
#include <cmath>

namespace boat::geometry {

template <class T>
using as = d2<typename boost::geometry::coordinate_system<T>::type>;

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
concept tagged = has_tag<T>::value;

template <class T>
concept ogc99 = tagged<T> && std::convertible_to<T, typename as<T>::variant>;

template <class T>
concept point = std::same_as<tag<T>, boost::geometry::point_tag>;

template <class T>
concept polygon = std::same_as<tag<T>, boost::geometry::polygon_tag>;

template <class T, class U>
concept same_tag = tagged<T> && std::same_as<tag<T>, tag<U>>;

template <class T>
concept segment = std::same_as<tag<T>, boost::geometry::segment_tag>;

template <class T>
concept single = std::derived_from<tag<T>, boost::geometry::single_tag>;

template <class T>
concept curve = single<T> && point<std::ranges::range_value_t<T>>;

template <class T>
concept projection_or_transformation =
    specialized<T, boost::geometry::srs::projection> ||
    specialized<T, boost::geometry::srs::transformation>;

template <class T>
concept srs_spec =
    std::constructible_from<boost::geometry::srs::projection<>, T>;

template <class... Ts>
void variant_emplace(std::variant<Ts...>& var, size_t i)
{
    check(i < sizeof...(Ts), "variant_emplace");
    static std::variant<Ts...> const vars[] = {Ts{}...};
    var = vars[i];
}

template <specialized<std::variant> V, class T, size_t I = 0>
constexpr size_t variant_index()
{
    if constexpr (std::same_as<std::variant_alternative_t<I, V>, T>)
        return I;
    else
        return variant_index<V, T, I + 1>();
}

template <class T>
constexpr size_t variant_index_v = variant_index<typename as<T>::variant, T>();

auto frac(std::floating_point auto v)
{
    return std::modf(v, &v);
}

template <arithmetic T>
T circular_clamp(T v, T lo, T hi)
{
    if constexpr (std::integral<T>)
        v = (v - lo) % (hi - lo);
    else
        v = std::fmod(v - lo, hi - lo);
    return v + (v < 0 ? hi : lo);
}

template <arithmetic T>
std::pair<T, bool> mirrored_clamp(T v, T lo, T hi)
{
    v = circular_clamp(v, lo, 2 * hi - lo);
    return {v > hi ? 2 * hi - v : v, v > hi};
}

}  // namespace boat::geometry

#endif  // BOAT_GEOMETRY_UTILITY_HPP
