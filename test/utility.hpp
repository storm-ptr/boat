// Andrew Naplavkov

#ifndef BOAT_TEST_UTILITY_HPP
#define BOAT_TEST_UTILITY_HPP

#include <boat/detail/numbers.hpp>
#include <boat/geometry/vocabulary.hpp>
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
    F f;
    Arg arg;
    revoke(F f, Arg const& arg) : f{f}, arg{std::invoke(f, arg)} {}
    ~revoke() { std::invoke(f, arg); }
};

template <std::floating_point T>
T circular_clamp(T v, T lo, T hi)
{
    v = std::fmod(v - lo, hi - lo);
    return v + (v < 0 ? hi : lo);
}

template <std::floating_point T>
std::pair<T, bool> mirrored_clamp(T v, T lo, T hi)
{
    auto two_hi = 2 * hi;
    v = circular_clamp(v, lo, two_hi - lo);
    auto mir = v > hi;
    return {mir ? two_hi - v : v, mir};
}

inline boat::geometry::geographic::point add_meters(  //
    boat::geometry::geographic::point const& p,
    double eastward,
    double northward)
{
    namespace num = boat::numbers;
    auto meter = num::radian / num::earth::mean_radius;
    auto dy = northward * meter;
    auto [y, mir] = mirrored_clamp(p.y() + dy, -90., 90.);
    auto den = std::cos(p.y() * num::degree);
    auto dx = den ? eastward * meter / den : 0.;
    auto x = circular_clamp(p.x() + dx + (mir ? 180 : 0), -180., 180.);
    return {x, y};
}

inline std::generator<boat::geometry::geographic::point> geographic_random()
{
    namespace num = boat::numbers;
    auto gen = std::mt19937{std::random_device()()};
    auto dist = std::uniform_real_distribution<double>{0, 1};
    for (;;) {
        auto azimuth = 2 * num::pi * dist(gen);
        auto polar = std::acos(1 - 2 * dist(gen));
        co_yield {azimuth * num::radian - 180, polar * num::radian - 90};
    }
}

#define BOAT_LIFT(f) \
    []<class... Args>(Args&&... args) { return f(std::forward<Args>(args)...); }

#endif  // BOAT_TEST_UTILITY_HPP
