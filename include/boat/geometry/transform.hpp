// Andrew Naplavkov

#ifndef BOAT_GEOMETRY_TRANSFORM_HPP
#define BOAT_GEOMETRY_TRANSFORM_HPP

#include <boat/detail/numbers.hpp>
#include <boat/geometry/vocabulary.hpp>
#include <boost/geometry/algorithms/transform.hpp>
#include <boost/geometry/srs/epsg.hpp>
#include <boost/geometry/srs/transformation.hpp>
#include <boost/geometry/strategies/transform/matrix_transformers.hpp>
#include <boost/geometry/strategies/transform/srs_transformer.hpp>
#include <format>
#include <optional>

namespace boat::geometry {

namespace srs = boost::geometry::srs;

using srs_variant = std::variant<srs::epsg, srs::proj4>;

template <class T>
constexpr bool is_srs_spec = std::is_constructible_v<srs::projection<>, T>;

template <class... Ts>
constexpr bool is_srs_spec<std::variant<Ts...>> = (is_srs_spec<Ts> && ...);

template <class T>
concept srs_spec = is_srs_spec<std::remove_cvref_t<T>>;

template <class T>
concept projection_or_transformation =
    specialized<T, srs::projection> || specialized<T, srs::transformation>;

static auto const lonlat = srs::proj4{" +proj=lonlat +datum=WGS84 +no_defs"};

inline auto ortho(geographic::point const& v)
{
    return srs::proj4{std::format(
        " +proj=ortho +x_0=0 +y_0=0 +units=m +no_defs"
        " +a={} +b={} +lat_0={} +lon_0={}",
        numbers::earth::equatorial_radius,
        numbers::earth::polar_radius,
        v.y(),
        v.x())};
}

auto transformation(srs_spec auto const& a, srs_spec auto const& b)
{
    if constexpr (specialized<decltype(a), std::variant>)
        return std::visit([&](auto&& a) { return transformation(a, b); }, a);
    else if constexpr (specialized<decltype(b), std::variant>)
        return std::visit([&](auto&& b) { return transformation(a, b); }, b);
    else
        return srs::transformation<>(a, b);
}

/// From WGS 84 lon/lat to the specified CRS.
auto transformation(srs_spec auto const& v)
{
    return transformation(lonlat, v);
}

inline auto forward(matrix const& v)
{
    return boost::geometry::strategy::transform::
        matrix_transformer<double, 2, 2>{v};
}

template <projection_or_transformation T>
auto forward(T const& v)
{
    return boost::geometry::strategy::transform::srs_forward_transformer<T>{v};
}

inline auto backward(matrix const& v)
{
    return forward(inverse(v));
}

template <projection_or_transformation T>
auto backward(T const& v)
{
    return boost::geometry::strategy::transform::srs_inverse_transformer<T>{v};
}

/// Collections succeed if any element succeeds; failed elements are omitted.
template <tagged T1, same_tag<T1> T2, class Strategy>
bool transform(T1 const& geom1, T2& geom2, Strategy const& strategy)
{
    return overloaded{
        [&](single auto const& a, single auto& b) {
            return boost::geometry::transform(a, b = {}, strategy);
        },
        [](this auto&& self, multi auto const& a, multi auto& b) -> bool {
            b = {};
            for (auto const& v : a)
                if (!self(v, b.emplace_back()))
                    b.pop_back();
            return !b.empty();
        },
        [](this auto&& self, dynamic auto const& a, dynamic auto& b) -> bool {
            auto vis = [&]<class T>(T const& v) {
                return self(v, b.template emplace<variant_index_v<T>>());
            };
            return std::visit(vis, a);
        }}(geom1, geom2);
}

/// Strategies run left to right; a failed step returns std::nullopt.
auto transform(auto const&... strategies)
{
    return [=]<tagged T>(T a) {
        T b;
        return (... && (b = std::move(a), transform(b, a, strategies)))
                   ? std::optional{std::move(a)}
                   : std::nullopt;
    };
}

/// Prefers positive EPSG to PROJ.4; throws if neither is available.
inline srs_variant make_srs_variant(int epsg, std::string const& proj4)
{
    if (epsg > 0)
        return srs::epsg{epsg};
    if (!proj4.empty())
        return srs::proj4{proj4};
    throw std::runtime_error("no SRS");
}

}  // namespace boat::geometry

#endif  // BOAT_GEOMETRY_TRANSFORM_HPP
