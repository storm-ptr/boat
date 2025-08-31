// Andrew Naplavkov

#ifndef BOAT_SLIPPY_UTILITY_HPP
#define BOAT_SLIPPY_UTILITY_HPP

#include <boat/slippy/tile.hpp>
#include <climits>

namespace boat::slippy {

inline auto const steps = std::views::iota(0, zmax) |
                          std::views::transform([](int z) {
                              return 2 * pi * 6'378'137 / std::pow(2, z);
                          }) |
                          std::ranges::to<std::vector>();

inline double step_factor(double lat)
{
    return std::cos(lat * degree);
}

inline int zoom(double step, double lat)
{
    auto ret = 0;
    auto k = step_factor(lat);
    auto min = DBL_MAX;
    for (int z{}; z < steps.size(); ++z)
        if (auto err = std::fabs(step - steps[z] * k); err < min)
            ret = z, min = err;
    return ret;
}

inline int lon2tilex(double lon, int z)
{
    return static_cast<int>((lon + 180) / 360 * std::pow(2, z));
}

inline int lat2tiley(double lat, int z)
{
    return static_cast<int>((1 - std::asinh(std::tan(lat * degree)) / pi) *
                            std::pow(2, z - 1));
}

inline double tilex2lon(int x, int z)
{
    return x / std::pow(2, z) * 360 - 180;
}

inline double tiley2lat(int y, int z)
{
    auto n = pi * (1 - y / std::pow(2, z - 1));
    return std::atan((std::exp(n) - std::exp(-n)) / 2) * radian;
}

inline tile corrected(tile const& t)
{
    int n = static_cast<int>(std::pow(2, t.z));
    return {(t.x < 0 ? n : 0) + t.x % n, (t.y < 0 ? n : 0) + t.y % n, t.z};
}

}  // namespace boat::slippy

#endif  // BOAT_SLIPPY_UTILITY_HPP
