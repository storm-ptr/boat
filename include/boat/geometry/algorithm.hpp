// Andrew Naplavkov

#ifndef BOAT_GEOMETRY_ALGORITHM_HPP
#define BOAT_GEOMETRY_ALGORITHM_HPP

#include <boat/geometry/detail/utility.hpp>
#include <climits>

namespace boat::geometry {

inline auto buffer(double distance, size_t num_points)
{
    return [=]<ogc99 Geom>(Geom const& geom) {
        namespace strategy = boost::geometry::strategy::buffer;
        using strategy_point_circle = std::conditional_t<
            std::same_as<typename boost::geometry::cs_tag<Geom>::type,
                         boost::geometry::geographic_tag>,
            strategy::geographic_point_circle<>,
            strategy::point_circle>;
        auto ret = typename d2<coord_sys<Geom>>::multi_polygon{};
        boost::geometry::buffer(geom,
                                ret,
                                strategy::distance_symmetric{distance},
                                strategy::side_straight{},
                                strategy::join_round{num_points},
                                strategy::end_round{num_points},
                                strategy_point_circle{num_points});
        if constexpr (multi<Geom>)
            return ret;
        else
            return std::move(ret.at(0));
    };
}

template <ogc99 Geom>
d2<coord_sys<Geom>>::box envelope(Geom const& geom)
{
    auto xmin = DBL_MAX;
    auto xmax = -DBL_MAX;
    auto ymin = DBL_MAX;
    auto ymax = -DBL_MAX;
    overloaded{[&](single auto& g) {
                   boost::geometry::for_each_point(g, [&](point auto& p) {
                       xmin = std::min<>(xmin, p.x());
                       xmax = std::max<>(xmax, p.x());
                       ymin = std::min<>(ymin, p.y());
                       ymax = std::max<>(ymax, p.y());
                   });
               },
               [](this auto&& self, multi auto& g) -> void {
                   std::ranges::for_each(g, self);
               },
               [](this auto&& self, dynamic auto& g) -> void {
                   std::visit(self, g);
               }}(geom);
    return {{xmin, ymin}, {xmax, ymax}};
}

template <box Box>
d2<coord_sys<Box>>::polygon to_polygon(Box const& mbr)
{
    auto ret = typename d2<coord_sys<Box>>::polygon{};
    boost::geometry::convert(mbr, ret);
    return ret;
}

}  // namespace boat::geometry

#endif  // BOAT_GEOMETRY_ALGORITHM_HPP
