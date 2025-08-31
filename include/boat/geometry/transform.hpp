// Andrew Naplavkov

#ifndef BOAT_GEOMETRY_TRANSFORM_HPP
#define BOAT_GEOMETRY_TRANSFORM_HPP

#include <boat/geometry/detail/distribution.hpp>
#include <boost/geometry/strategies/transform/srs_transformer.hpp>
#include <optional>

namespace boat::geometry {

template <ogc99_or_box Geom1, same_tag<Geom1> Geom2, class Strategy>
bool transform(Geom1 const& geom1, Geom2& geom2, Strategy const& strategy)
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
            auto vis = [&]<class G>(G const& g) {
                return self(g, g2.template emplace<variant_index_v<G>>());
            };
            return std::visit(vis, g1);
        },
        []<box B1, box B2>(this auto&& self, B1 const& b1, B2& b2) -> bool {
            auto mp2 = typename d2<coord_sys<B2>>::multi_point{};
            for (auto& p1 : box_interpolate(b1, 7))
                if (!self(p1, mp2.emplace_back()))
                    mp2.pop_back();
            b2 = envelope(mp2);
            return !mp2.empty();
        }}(geom1, geom2);
}

auto transform(auto const&... strategies)
{
    return [=]<ogc99_or_box Geom>(Geom geom) {
        Geom tmp;
        return (... && (std::swap(geom, tmp), transform(tmp, geom, strategies)))
                   ? std::optional{std::move(geom)}
                   : std::nullopt;
    };
}

template <projection_or_transformation T>
auto forward(T const& tf)
{
    return boost::geometry::strategy::transform::srs_forward_transformer<T>{tf};
}

template <projection_or_transformation T>
auto inverse(T const& tf)
{
    return boost::geometry::strategy::transform::srs_inverse_transformer<T>{tf};
}

auto forward(box auto const& mbr, int width, int height)
{
    return boost::geometry::strategy::transform::
        map_transformer<double, 2, 2, true, false>{mbr, width, height};
}

auto inverse(box auto const& mbr, int width, int height)
{
    return boost::geometry::strategy::transform::
        inverse_transformer<double, 2, 2>{forward(mbr, width, height)};
}

}  // namespace boat::geometry

#endif  // BOAT_GEOMETRY_TRANSFORM_HPP
