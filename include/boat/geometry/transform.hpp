// Andrew Naplavkov

#ifndef BOAT_GEOMETRY_TRANSFORM_HPP
#define BOAT_GEOMETRY_TRANSFORM_HPP

#include <boat/detail/numbers.hpp>
#include <boat/detail/string.hpp>
#include <boat/geometry/vocabulary.hpp>
#include <boost/geometry/strategies/transform/srs_transformer.hpp>
#include <optional>

namespace boat::geometry {

static auto const lonlat = srs::proj4{" +proj=lonlat +datum=WGS84 +no_defs"};

inline auto ortho(geographic::point const& v)
{
    return srs::proj4{concat(  //
        " +proj=ortho +x_0=0 +y_0=0 +units=m +no_defs +a=",
        numbers::earth::equatorial_radius,
        " +b=",
        numbers::earth::polar_radius,
        " +lat_0=",
        v.y(),
        " +lon_0=",
        v.x())};
}

inline srs_variant to_srs_variant(auto const& meta)
{
    return meta.epsg > 0         ? srs_variant{srs::epsg{meta.epsg}}
           : !meta.proj4.empty() ? srs_variant{srs::proj4{meta.proj4}}
                                 : throw std::runtime_error("no SRS");
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

auto transformation(srs_spec auto const& v)
{
    return transformation(lonlat, v);
}

template <projection_or_transformation T>
auto srs_forward(T const& v)
{
    return boost::geometry::strategy::transform::srs_forward_transformer<T>{v};
}

template <projection_or_transformation T>
auto srs_inverse(T const& v)
{
    return boost::geometry::strategy::transform::srs_inverse_transformer<T>{v};
}

using mat_forward =
    boost::geometry::strategy::transform::matrix_transformer<double, 2, 2>;

inline auto mat_inverse(matrix const& v)
{
    return mat_forward{inverse(v)};
}

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

auto transform(auto const&... strategies)
{
    return [=]<tagged T>(T a) {
        T b;
        return (... && (b = std::move(a), transform(b, a, strategies)))
                   ? std::optional{std::move(a)}
                   : std::nullopt;
    };
}

}  // namespace boat::geometry

#endif  // BOAT_GEOMETRY_TRANSFORM_HPP
