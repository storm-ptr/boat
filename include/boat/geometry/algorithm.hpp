// Andrew Naplavkov

#ifndef BOAT_GEOMETRY_ALGORITHM_HPP
#define BOAT_GEOMETRY_ALGORITHM_HPP

#include <boat/geometry/detail/utility.hpp>

namespace boat::geometry {

inline auto buffer(double distance, size_t num_points)
{
    return [=]<tagged T>(T const& geom) {
        namespace strategy = boost::geometry::strategy::buffer;
        using strategy_point_circle = std::conditional_t<
            std::same_as<typename boost::geometry::cs_tag<T>::type,
                         boost::geometry::geographic_tag>,
            strategy::geographic_point_circle<>,
            strategy::point_circle>;
        auto ret = typename as<T>::multi_polygon{};
        boost::geometry::buffer(geom,
                                ret,
                                strategy::distance_symmetric{distance},
                                strategy::side_straight{},
                                strategy::join_round{num_points},
                                strategy::end_round{num_points},
                                strategy_point_circle{num_points});
        if constexpr (multi<T>)
            return ret;
        else
            return std::move(ret.at(0));
    };
}

constexpr auto minmax = []<tagged T>(T const& geom) -> as<T>::box {
    double xmin = INFINITY;
    double ymin = INFINITY;
    double xmax = -INFINITY;
    double ymax = -INFINITY;
    overloaded{[&](single auto& g) {
                   boost::geometry::for_each_point(g, [&](point auto& p) {
                       xmin = std::min<>(xmin, p.x());
                       ymin = std::min<>(ymin, p.y());
                       xmax = std::max<>(xmax, p.x());
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
};

}  // namespace boat::geometry

#endif  // BOAT_GEOMETRY_ALGORITHM_HPP
