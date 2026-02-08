// Andrew Naplavkov

#ifndef BOAT_GEOMETRY_TYPE_TRAITS_HPP
#define BOAT_GEOMETRY_TYPE_TRAITS_HPP

#include <boat/detail/utility.hpp>
#include <boat/geometry/vocabulary.hpp>

namespace boat::geometry {

template <class T>
using d2_of = d2<typename boost::geometry::coordinate_system<T>::type>;

template <class T>
using tag = boost::geometry::tag<T>::type;

template <class T>
struct has_tag : std::negation<std::is_same<tag<T>, void>> {};

template <class... Ts>
struct has_tag<std::variant<Ts...>> : std::conjunction<has_tag<Ts>...> {};

template <class T>
constexpr size_t variant_index_v =
    variant_index<typename d2_of<T>::variant, T>();

}  // namespace boat::geometry

#endif  // BOAT_GEOMETRY_TYPE_TRAITS_HPP
