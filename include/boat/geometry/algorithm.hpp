// Andrew Naplavkov

#ifndef BOAT_GEOMETRY_ALGORITHM_HPP
#define BOAT_GEOMETRY_ALGORITHM_HPP

#include <boat/detail/numbers.hpp>
#include <boat/geometry/vocabulary.hpp>
#include <boost/geometry.hpp>

namespace boat::geometry {

template <box T>
multi_point auto
box_interpolate(T const& mbr, size_t edge_points, size_t area_points)
{
    auto ret = typename d2_t<T>::multi_point{};
    for (auto edge : boost::geometry::box_view{mbr} | std::views::pairwise) {
        auto a = std::get<0>(edge), b = std::get<1>(edge);
        for (auto i : std::views::iota(0uz, edge_points)) {
            auto t = double(i) / edge_points;
            ret.emplace_back(
                std::lerp(a.x(), b.x(), t), std::lerp(a.y(), b.y(), t));
        }
    }
    auto a = mbr.min_corner(), b = mbr.max_corner();
    for (auto i : std::views::iota(0uz, area_points))
        ret.emplace_back(
            std::lerp(a.x(), b.x(), frac(i * numbers::inv_phi)),
            std::lerp(a.y(), b.y(), (i + .5) / area_points));
    return ret;
}

inline auto cartesian_envelope(geographic::multi_point const& points)
{
    return return_envelope<cartesian::box>(
        points, boost::geometry::strategies::envelope::cartesian<>{});
}

inline auto buffer(double distance, size_t num_points)
{
    return [=]<single T>(T const& geom) -> polygon auto {
        namespace sb = boost::geometry::strategy::buffer;
        using point_circle = std::conditional_t<
            std::same_as<
                boost::geometry::cs_tag_t<T>,
                boost::geometry::geographic_tag>,
            sb::geographic_point_circle<>,
            sb::point_circle>;
        auto out = typename d2_t<T>::multi_polygon{};
        boost::geometry::buffer(
            geom,
            out,
            sb::distance_symmetric{distance},
            sb::side_straight{},
            sb::join_round{num_points},
            sb::end_round{num_points},
            point_circle{num_points});
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

inline std::vector<geographic::box> split_at_antimeridian(
    geographic::box const& box)
{
    auto a = box.min_corner(), b = box.max_corner();
    if (a.x() < -180.) {
        a.x(a.x() + 360.);
        b.x(b.x() + 360.);
    }
    if (b.x() <= 180.)
        return {{a, b}};
    return {{a, {180., b.y()}}, {{-180., a.y()}, {b.x() - 360., b.y()}}};
}

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

/// Wraps at the poles and antimeridian.
inline geographic::point wrap(geographic::point const& p)
{
    auto [y, reflected] = reflect(p.y(), -90., 90.);
    return {boat::wrap(p.x() + 180 * reflected, -180., 180.), y};
}

inline geographic::point
add_meters(geographic::point const& p, double eastward, double northward)
{
    return wrap(geographic::point{
        p.x() + eastward * meter(p.y()), p.y() + northward * meter()});
}

}  // namespace boat::geometry

#endif  // BOAT_GEOMETRY_ALGORITHM_HPP
