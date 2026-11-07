// Andrew Naplavkov

#ifndef BOAT_GEOMETRY_ALGORITHM_HPP
#define BOAT_GEOMETRY_ALGORITHM_HPP

#include <boat/detail/numbers.hpp>
#include <boat/geometry/vocabulary.hpp>
#include <boost/geometry.hpp>

namespace boat::geometry {

template <box T>
auto box_area_interpolate(T const& mbr, size_t num_points)
{
    auto a = mbr.min_corner(), b = mbr.max_corner();
    auto fn = [=](auto i) -> d2_of<T>::point {
        return {
            std::lerp(a.x(), b.x(), frac(i * numbers::inv_phi)),
            std::lerp(a.y(), b.y(), (i + .5) / num_points)};
    };
    return std::views::iota(0uz, num_points) | std::views::transform(fn);
}

template <box T>
multi_point auto box_edge_interpolate(T const& mbr, size_t num_points)
{
    auto ret = typename d2_of<T>::multi_point{};
    for (auto edge : boost::geometry::box_view{mbr} | std::views::pairwise) {
        auto a = std::get<0>(edge), b = std::get<1>(edge);
        for (auto i : std::views::iota(0uz, num_points + 1)) {
            auto t = i / (num_points + 1.);
            ret.emplace_back(
                std::lerp(a.x(), b.x(), t), std::lerp(a.y(), b.y(), t));
        }
    }
    return ret;
}

inline auto buffer(double distance, size_t num_points)
{
    return [=]<single T>(T const& geom) -> polygon auto {
        namespace strategy = boost::geometry::strategy::buffer;
        using strategy_point_circle = std::conditional_t<
            std::same_as<
                typename boost::geometry::cs_tag<T>::type,
                boost::geometry::geographic_tag>,
            strategy::geographic_point_circle<>,
            strategy::point_circle>;
        auto out = typename d2_of<T>::multi_polygon{};
        boost::geometry::buffer(
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

/// Approximate degrees per meter of latitude, or longitude at a given latitude.
constexpr auto meter = overloaded{
    [] { return numbers::radian / numbers::earth::mean_radius; },
    [](this auto&& self, double lat) {
        auto den = std::cos(lat * numbers::degree);
        return den ? self() / den : 0.;
    }};

auto visitor(auto... fn)
{
    return overloaded{
        fn...,
        [](this auto&& self, multi auto&& v) -> void {
            std::ranges::for_each(v, self);
        },
        [](this auto&& self, dynamic auto&& v) -> void {
            std::visit(self, v);
        }};
}

void for_each_point(tagged auto const& geom, auto fn)
{
    visitor([&](single auto&& v) { boost::geometry::for_each_point(v, fn); })(
        geom);
}

constexpr auto minmax = []<tagged T>(T const& geom) -> box auto {
    double xmin = INFINITY;
    double ymin = INFINITY;
    double xmax = -INFINITY;
    double ymax = -INFINITY;
    geometry::for_each_point(geom, [&](point auto&& p) {
        xmin = std::min<>(xmin, p.x());
        ymin = std::min<>(ymin, p.y());
        xmax = std::max<>(xmax, p.x());
        ymax = std::max<>(ymax, p.y());
    });
    return typename d2_of<T>::box{{xmin, ymin}, {xmax, ymax}};
};

template <box T>
polygon auto to_polygon(T const& geom)
{
    typename d2_of<T>::polygon ret;
    boost::geometry::convert(geom, ret);
    return ret;
}

/// Wraps at the poles and antimeridian.
inline geographic::point wrap(geographic::point const& p)
{
    auto [y, reflected] = reflect(p.y(), -90., 90.);
    auto x = boat::wrap(p.x() + 180 * reflected, -180., 180.);
    return {x, y};
}

inline geographic::point
add_meters(geographic::point const& p, double eastward, double northward)
{
    auto dx = eastward * meter(p.y());
    auto dy = northward * meter();
    return wrap(geographic::point{p.x() + dx, p.y() + dy});
}

}  // namespace boat::geometry

#endif  // BOAT_GEOMETRY_ALGORITHM_HPP
