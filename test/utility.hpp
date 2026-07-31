// Andrew Naplavkov

#ifndef BOAT_TEST_UTILITY_HPP
#define BOAT_TEST_UTILITY_HPP

#include <boat/geometry/algorithm.hpp>
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
