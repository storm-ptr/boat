// Andrew Naplavkov

#ifndef BOAT_GEOMETRY_RASTER_HPP
#define BOAT_GEOMETRY_RASTER_HPP

#include <boat/geometry/algorithm.hpp>
#include <boat/geometry/detail/fibonacci.hpp>
#include <boat/geometry/transform.hpp>
#include <boost/qvm/map_vec_mat.hpp>

namespace boat::geometry {

/// Maps pixels to spatial coordinates.
inline matrix affine(int width, int height, cartesian::segment const& mid_pixel)
{
    namespace qvm = boost::qvm;
    auto const& [a, b] = mid_pixel;
    auto scale = boost::geometry::distance(a, b);
    return qvm::translation_mat(qvm::vec{{a.x(), a.y()}}) *
           qvm::rotz_mat<3>(.5 * numbers::pi - boost::geometry::azimuth(a, b)) *
           qvm::diag_mat(qvm::vec{{scale, -scale, 1.}}) *
           qvm::translation_mat(-qvm::vec{{width * .5, height * .5}});
}

/// Maps geographic coordinates to pixels and back.
auto bidirectional_transform(matrix const& mat, srs_spec auto const& crs)
{
    auto tf = transformation(crs);
    return std::pair{
        transform(srs_forward(tf), mat_inverse(mat)),
        transform(mat_forward(mat), srs_inverse(tf)),
    };
}

/// Maps raster 1 pixels to raster 2 pixels and back.
auto bidirectional_transform(  //
    matrix const& affine1,
    srs_spec auto const& crs1,
    matrix const& affine2,
    srs_spec auto const& crs2)
{
    auto tf = transformation(crs1, crs2);
    return std::pair{
        transform(mat_forward(affine1), srs_forward(tf), mat_inverse(affine2)),
        transform(mat_forward(affine2), srs_inverse(tf), mat_inverse(affine1)),
    };
}

/// Samples a raster footprint on a fixed global grid.
geographic::grid geographic_interpolate(  //
    int width,
    int height,
    matrix const& mat,
    srs_spec auto const& crs,
    size_t num_points,
    double margin = .05)
{
    auto [fwd, inv] = bidirectional_transform(mat, crs);
    auto dx = width * margin, dy = height * margin;
    auto bounds = cartesian::box{{-dx, -dy}, {width + dx, height + dy}};
    auto sentinel = [&](auto& ll) {
        auto xy = fwd(ll).transform(cartesian{});
        return !xy || !boost::geometry::covered_by(*xy, bounds);
    };
    auto points =
        inv(box_area_interpolate(geographic::box{{}, {width * 1., height * 1.}},
                                 num_points) |
            std::ranges::to<geographic::multi_point>())
            .value_or(geographic::multi_point{});
    auto ret = geographic::grid{};
    for (auto fib : fibonacci_levels) {
        auto indices = std::unordered_set<size_t>{};
        for (auto& p : points)
            for (auto i : fib.nearests(p, sentinel)) {
                if (!indices.insert(i).second)
                    break;
                if (indices.size() > points.size() * 2)
                    return ret;
            }
        if (indices.empty())
            continue;
        ret[numbers::earth::sqrt_area / std::sqrt(fib.num_points)].assign_range(
            indices | std::views::transform([&](size_t i) { return fib[i]; }));
    }
    return ret;
}

}  // namespace boat::geometry

#endif  // BOAT_GEOMETRY_RASTER_HPP
