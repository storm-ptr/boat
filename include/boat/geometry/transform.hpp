// Andrew Naplavkov

#ifndef BOAT_GEOMETRY_TRANSFORM_HPP
#define BOAT_GEOMETRY_TRANSFORM_HPP

#include <boat/detail/charconv.hpp>
#include <boat/detail/numbers.hpp>
#include <boat/geometry/concepts.hpp>
#include <boost/geometry/srs/epsg.hpp>
#include <boost/geometry/strategies/transform/srs_transformer.hpp>
#include <optional>

namespace boat::geometry {

static auto const lonlat =
    boost::geometry::srs::proj4{" +proj=lonlat +datum=WGS84 +no_defs"};

inline auto ortho(geographic::point const& center)
{
    return boost::geometry::srs::proj4{concat(  //
        " +proj=ortho +x_0=0 +y_0=0 +units=m +no_defs +a=",
        numbers::earth::equatorial_radius,
        " +b=",
        numbers::earth::polar_radius,
        " +lat_0=",
        center.y(),
        " +lon_0=",
        center.x())};
}

auto transformation(srs_spec auto const& srs)
{
    return boost::geometry::srs::transformation<>(lonlat, srs);
}

template <projection_or_transformation T>
auto srs_forward(T const& tf)
{
    return boost::geometry::strategy::transform::srs_forward_transformer<T>{tf};
}

template <projection_or_transformation T>
auto srs_inverse(T const& tf)
{
    return boost::geometry::strategy::transform::srs_inverse_transformer<T>{tf};
}

using mat_forward =
    boost::geometry::strategy::transform::matrix_transformer<double, 2, 2>;

inline auto mat_inverse(matrix const& mat)
{
    return mat_forward{boost::qvm::inverse(mat)};
}

template <tagged T1, same_tag<T1> T2, class Strategy>
bool transform(T1 const& geom1, T2& geom2, Strategy const& strategy)
{
    return overloaded{
        [&](single auto const& g1, single auto& g2) {
            return boost::geometry::transform(g1, g2 = {}, strategy);
        },
        [](this auto&& self, multi auto const& g1, multi auto& g2) -> bool {
            g2 = {};
            for (auto const& g : g1)
                if (!self(g, g2.emplace_back()))
                    g2.pop_back();
            return !g2.empty();
        },
        [](this auto&& self, dynamic auto const& g1, dynamic auto& g2) -> bool {
            auto vis = [&]<class T>(T const& g) {
                return self(g, g2.template emplace<variant_index_v<T>>());
            };
            return std::visit(vis, g1);
        }}(geom1, geom2);
}

auto transform(auto const&... strategies)
{
    return [=]<tagged T>(T g1) {
        T g2;
        return (... && (g2 = std::move(g1), transform(g2, g1, strategies)))
                   ? std::optional{std::move(g1)}
                   : std::nullopt;
    };
}

}  // namespace boat::geometry

#endif  // BOAT_GEOMETRY_TRANSFORM_HPP
