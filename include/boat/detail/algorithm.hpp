// Andrew Naplavkov

#ifndef BOAT_ALGORITHM_HPP
#define BOAT_ALGORITHM_HPP

#include <boat/detail/utility.hpp>
#include <numeric>
#include <optional>
#include <vector>

namespace boat {

template <arithmetic T>
T circular_clamp(T v, T lo, T hi)
{
    if constexpr (std::integral<T>)
        v = (v - lo) % (hi - lo);
    else
        v = std::fmod(v - lo, hi - lo);
    return v + (v < 0 ? hi : lo);
}

template <arithmetic T>
std::pair<T, bool> mirrored_clamp(T v, T lo, T hi)
{
    auto two_hi = 2 * hi;
    v = circular_clamp(v, lo, two_hi - lo);
    auto mir = v > hi;
    return {mir ? two_hi - v : v, mir};
}

template <std::integral T>
inline T iexp2(double scale, T hi)
{
    auto n = scale * pow2(hi);
    for (T i = hi; i > 0; --i)
        if (n >= pow2(i))
            return i;
    return 0;
}

template <arithmetic T, std::integral E>
T ipow(T base, E exp)
{
    if constexpr (std::signed_integral<E>)
        check(exp >= 0, "ipow");
    T ret = 1;
    for (;;) {
        if (exp & 1)
            ret *= base;
        exp >>= 1;
        if (!exp)
            break;
        base *= base;
    }
    return ret;
}

}  // namespace boat

#endif  // BOAT_ALGORITHM_HPP
