// Andrew Naplavkov

#ifndef BOAT_GEOMETRY_ALGORITHM_HPP
#define BOAT_GEOMETRY_ALGORITHM_HPP

#include <boat/detail/numbers.hpp>
#include <boat/geometry/vocabulary.hpp>

namespace boat::geometry {

template <point T>
T add_value(T geom, double value)
{
    boost::geometry::add_value(geom, value);
    return geom;
}

inline auto buffer(double distance, size_t num_points)
{
    return [=]<single T>(T const& geom) -> polygon auto {
        namespace strategy = boost::geometry::strategy::buffer;
        using strategy_point_circle = std::conditional_t<
            std::same_as<typename boost::geometry::cs_tag<T>::type,
                         boost::geometry::geographic_tag>,
            strategy::geographic_point_circle<>,
            strategy::point_circle>;
        auto out = typename d2_of<T>::multi_polygon{};
        boost::geometry::buffer(  //
            geom,
            out,
            strategy::distance_symmetric{distance},
            strategy::side_straight{},
            strategy::join_round{num_points},
            strategy::end_round{num_points},
            strategy_point_circle{num_points});
        return std::move(out.at(0));
    };
}

constexpr auto minmax = []<tagged T>(T const& geom) -> box auto {
    double xmin = INFINITY;
    double ymin = INFINITY;
    double xmax = -INFINITY;
    double ymax = -INFINITY;
    overloaded{
        [&](single auto& g) {
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
        [](this auto&& self, dynamic auto& var) -> void {
            std::visit(self, var);
        },
    }(geom);
    return typename d2_of<T>::box{{xmin, ymin}, {xmax, ymax}};
};

template <box T>
polygon auto to_polygon(T const& geom)
{
    typename d2_of<T>::polygon ret;
    boost::geometry::convert(geom, ret);
    return ret;
}

inline geographic::point wrap(geographic::point const& p)
{
    auto [y, reflected] = reflect(p.y(), -90., 90.);
    auto x = boat::wrap(p.x() + 180 * reflected, -180., 180.);
    return {x, y};
}

inline geographic::point add_meters(  //
    geographic::point const& p,
    double eastward,
    double northward)
{
    constexpr auto meter = numbers::radian / numbers::earth::mean_radius;
    auto den = std::cos(p.y() * numbers::degree);
    auto dx = den ? eastward * meter / den : 0.;
    auto dy = northward * meter;
    return wrap(geographic::point{p.x() + dx, p.y() + dy});
}

}  // namespace boat::geometry

#endif  // BOAT_GEOMETRY_ALGORITHM_HPP
