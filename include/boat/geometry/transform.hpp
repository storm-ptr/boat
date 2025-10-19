// Andrew Naplavkov

#ifndef BOAT_GEOMETRY_TRANSFORM_HPP
#define BOAT_GEOMETRY_TRANSFORM_HPP

#include <boat/geometry/detail/distribution.hpp>
#include <boost/geometry/srs/epsg.hpp>
#include <boost/geometry/strategies/transform/srs_transformer.hpp>
#include <optional>

namespace boat::geometry {

template <ogc99_or_box T1, same_tag<T1> T2, class Strategy>
bool transform(T1 const& geom1, T2& geom2, Strategy const& strategy)
{
    return overloaded{
        [&](single auto const& g1, single auto& g2) {
            return boost::geometry::transform(g1, g2 = {}, strategy);
        },
        [](this auto&& self, multi auto const& g1, multi auto& g2) -> bool {
            g2.resize(g1.size());
            auto apply = [&](auto tuple) {
                return self(std::get<0>(tuple), std::get<1>(tuple));
            };
            return std::ranges::all_of(std::views::zip(g1, g2), apply);
        },
        [](this auto&& self, dynamic auto const& g1, dynamic auto& g2) -> bool {
            auto vis = [&]<class T>(T const& g) {
                return self(g, g2.template emplace<variant_index_v<T>>());
            };
            return std::visit(vis, g1);
        },
        []<box B1, box B2>(this auto&& self, B1 const& b1, B2& b2) -> bool {
            auto mp2 = typename as<B2>::multi_point{};
            for (auto const& p1 : box_interpolate(b1, 7))
                if (!self(p1, mp2.emplace_back()))
                    mp2.pop_back();
            for (auto const& p1 : box_fibonacci(b1, 37))
                if (!self(p1, mp2.emplace_back()))
                    mp2.pop_back();
            b2 = envelope(mp2);
            return !mp2.empty();
        }}(geom1, geom2);
}

auto transform(auto const&... strategies)
{
    return [=]<ogc99_or_box T>(T geom) {
        T tmp;
        return (... && (std::swap(geom, tmp), transform(tmp, geom, strategies)))
                   ? std::optional{std::move(geom)}
                   : std::nullopt;
    };
}

auto stable_projection(srs_params auto const& srs)
{
    return boost::geometry::srs::transformation<>(
        boost::geometry::srs::epsg{4326}, srs);
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

inline auto matrix(boost::qvm::mat<double, 3, 3> const& mat)
{
    return boost::geometry::strategy::transform::
        matrix_transformer<double, 2, 2>{mat};
}

}  // namespace boat::geometry

#endif  // BOAT_GEOMETRY_TRANSFORM_HPP
