// Andrew Naplavkov

#ifndef BOAT_TEST_UTILITY_HPP
#define BOAT_TEST_UTILITY_HPP

#include <boat/detail/algorithm.hpp>
#include <boat/detail/numbers.hpp>
#include <boat/geometry/concepts.hpp>
#include <generator>
#include <random>

namespace boost::geometry {

template <boat::geometry::ogc99 T>
bool operator==(T const& lhs, T const& rhs)
{
    return equals(lhs, rhs);
}

}  // namespace boost::geometry

template <class F, class Arg>
struct revoke {
    F f_;
    Arg arg_;
    revoke(F f, Arg const& arg) : f_{f}, arg_{std::invoke(f_, arg)} {}
    ~revoke() { std::invoke(f_, arg_); }
};

inline auto add_meters(boat::geometry::geographic::point const& p,
                       double eastward,
                       double northward)
{
    auto meter = boat::numbers::radian / boat::numbers::earth::mean_radius;
    auto dy = northward * meter;
    auto [y, mir] = boat::mirrored_clamp(p.y() + dy, -90., 90.);
    auto den = std::cos(p.y() * boat::numbers::degree);
    auto dx = (den ? eastward * meter / den : 0.);
    auto x = boat::circular_clamp(p.x() + dx + (mir ? 180. : 0.), -180., 180.);
    return boat::geometry::geographic::point{x, y};
}

inline std::generator<boat::geometry::geographic::point> geographic_random()
{
    auto gen = std::mt19937{std::random_device()()};
    auto dist = std::uniform_real_distribution<double>{0, 1};
    for (;;) {
        auto azimuthal = 2 * boat::numbers::pi * dist(gen);
        auto polar = std::acos(1 - 2 * dist(gen));
        co_yield {azimuthal * boat::numbers::radian - 180,
                  polar * boat::numbers::radian - 90};
    }
}

#define BOAT_LIFT(f) \
    []<class... Args>(Args&&... args) { return f(std::forward<Args>(args)...); }

#endif  // BOAT_TEST_UTILITY_HPP
