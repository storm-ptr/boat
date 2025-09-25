// Andrew Naplavkov

#ifndef BOAT_GEOMETRY_INVERSE_FIBONACCI_HPP
#define BOAT_GEOMETRY_INVERSE_FIBONACCI_HPP

#include <boat/geometry/detail/utility.hpp>
#include <boost/container/static_vector.hpp>
#include <boost/qvm/mat_operations.hpp>
#include <boost/qvm/vec_mat_operations.hpp>
#include <boost/qvm/vec_operations.hpp>

namespace boat::geometry {

template <class T = boost::container::static_vector<size_t, 4>>
T inverse_fibonacci(geographic::point const& point, size_t num_points)
{
    constexpr auto root_five = 2 * phi - 1;
    if (num_points <= 4)
        return std::views::iota(0u, num_points) | std::ranges::to<T>();
    auto azimuthal = (point.x() + 180) * degree;
    auto polar = (point.y() + 90) * degree;
    auto k =
        .5 * boost::math::double_constants::one_div_ln_phi *
        std::log(root_five * pi * num_points * std::pow(std::sin(polar), 2));
    k = std::max<>(2., std::floor(k));
    auto fk = std::pow(phi, static_cast<size_t>(k)) / root_five;
    auto f0 = std::round(fk);
    auto f1 = std::round(fk * phi);
    auto inv_size = 1. / num_points;
    auto b = boost::qvm::mat{{{2 * pi * (frac((f0 + 1) * inv_phi) - inv_phi),
                               2 * pi * (frac((f1 + 1) * inv_phi) - inv_phi)},
                              {-2 * f0 * inv_size, -2 * f1 * inv_size}}};
    auto c = boost::qvm::inverse(b) *
             boost::qvm::vec{{azimuthal, std::cos(polar) - 1 + inv_size}};
    X(c) = std::floor(X(c));
    Y(c) = std::floor(Y(c));
    auto ret = T{};
    for (double x : {0, 1})
        for (double y : {0, 1}) {
            auto corner = c + boost::qvm::vec{{x, y}};
            auto z = dot(row<1>(b), corner) + 1 - inv_size;
            z = 2 * std::clamp(z, -1., 1.) - z;
            ret.push_back(static_cast<size_t>(num_points * (1 - z) / 2));
        }
    return ret;
}

}  // namespace boat::geometry

#endif  // BOAT_GEOMETRY_INVERSE_FIBONACCI_HPP
