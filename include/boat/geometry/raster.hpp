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
    auto&& [a, b] = mid_pixel;
    auto scale = boost::geometry::distance(a, b);
    return qvm::translation_mat(qvm::vec{{a.x(), a.y()}}) *
           qvm::rotz_mat<3>(.5 * numbers::pi - boost::geometry::azimuth(a, b)) *
           qvm::diag_mat(qvm::vec{{scale, -scale, 1.}}) *
           qvm::translation_mat(-qvm::vec{{width * .5, height * .5}});
}

/// Maps lon/lat to pixels and back.
auto bidirectional_transform(matrix const& mat, srs_spec auto const& crs)
{
    auto tf = transformation(crs);
    return std::pair{
        transform(srs_forward(tf), mat_inverse(mat)),
        transform(mat_forward(mat), srs_inverse(tf))};
}

/// Maps between rasters.
auto bidirectional_transform(
    matrix const& affine1,
    srs_spec auto const& crs1,
    matrix const& affine2,
    srs_spec auto const& crs2)
{
    auto tf = transformation(crs1, crs2);
    return std::pair{
        transform(mat_forward(affine1), srs_forward(tf), mat_inverse(affine2)),
        transform(mat_forward(affine2), srs_inverse(tf), mat_inverse(affine1))};
}

template <box B, arithmetic I>
B bounds(I width, I height)
{
    return {{}, {width * 1., height * 1.}};
}

template <segment S, arithmetic I>
S pixel(I width, I height)
{
    auto x = width * .5, y = height * .5;
    return {{x, y}, {x + 1., y + 1.}};
}

/// Samples a fixed grid.
geographic::grid geographic_interpolate(
    int width,
    int height,
    matrix const& mat,
    srs_spec auto const& crs,
    double meters)
{
    auto inv = bidirectional_transform(mat, crs).second;
    auto px = inv(pixel<geographic::segment>(width, height));
    if (!px)
        return {};
    auto n =
        size_t(width * .5 * height * std::pow(length(*px) / meters, 2)) + 1;
    auto mbr = bounds<geographic::box>(width, height);
    auto points = box_edge_interpolate(mbr, size_t(2 * std::sqrt(n)));
    points.append_range(box_area_interpolate(mbr, n));
    points = inv(std::move(points)).value_or(geographic::multi_point{});
    auto ret = geographic::grid{};
    for (auto fib : fibonacci_levels) {
        auto step = fib.step();
        if (step < meters * numbers::inv_sqrt_2)
            break;
        auto sentinel = [&](auto& ll) { return distance(ll, points) > step; };
        auto indices = std::unordered_set<size_t>{};
        for (auto& p : points)
            for (auto i : fib.nearests(p, sentinel)) {
                if (!indices.insert(i).second)
                    break;
                ret[step].push_back(fib[i]);
            }
    }
    return ret;
}

}  // namespace boat::geometry

#endif  // BOAT_GEOMETRY_RASTER_HPP
