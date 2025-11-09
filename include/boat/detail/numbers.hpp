// Andrew Naplavkov

#ifndef BOAT_NUMBERS_HPP
#define BOAT_NUMBERS_HPP

#include <boost/math/constants/constants.hpp>

namespace boat::numbers {

using boost::math::double_constants::phi;
using boost::math::double_constants::pi;

constexpr auto deg = boost::math::double_constants::degree;
constexpr auto inv_ln_phi = boost::math::double_constants::one_div_ln_phi;
constexpr auto inv_phi = phi - 1;
constexpr auto rad = boost::math::double_constants::radian;
constexpr auto sqrt_5 = 2 * phi - 1;
constexpr auto sqrt_pi = boost::math::double_constants::root_pi;

namespace earth {

constexpr auto equatorial_radius = 6'378'137.;
constexpr auto polar_radius = 6'356'752.3142;
constexpr auto mean_radius = (2 * equatorial_radius + polar_radius) / 3;
constexpr auto sqrt_area = 2 * sqrt_pi * mean_radius;

}  // namespace earth
}  // namespace boat::numbers

#endif  // BOAT_NUMBERS_HPP
