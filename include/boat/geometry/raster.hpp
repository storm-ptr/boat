// Andrew Naplavkov

#ifndef BOAT_GEOMETRY_RASTER_HPP
#define BOAT_GEOMETRY_RASTER_HPP

#include <boat/geometry/algorithm.hpp>
#include <boat/geometry/detail/fibonacci.hpp>
#include <boat/geometry/transform.hpp>
#include <boost/qvm/map_vec_mat.hpp>

namespace boat::geometry {

/// Maps pixels to spatial coordinates.
inline matrix affine(raster::point ext, cartesian::segment const& mid_pixel)
{
    namespace qvm = boost::qvm;
    auto&& [a, b] = mid_pixel;
    auto scale = boost::geometry::distance(a, b);
    return qvm::translation_mat(qvm::vec{{a.x(), a.y()}}) *
           qvm::rotz_mat<3>(.5 * numbers::pi - boost::geometry::azimuth(a, b)) *
           qvm::diag_mat(qvm::vec{{scale, -scale, 1.}}) *
           qvm::translation_mat(-qvm::vec{{ext.x() * .5, ext.y() * .5}});
}

/// Maps pixels to spatial coordinates.
matrix affine(
    raster::point ext,
    geographic::point const& mid,
    double resolution,
    auto const& crs)
{
    if constexpr (projection_or_transformation<decltype(crs)>)
        return affine(ext, mid, resolution, transform(forward(crs)));
    else if constexpr (srs_spec<decltype(crs)>)
        return affine(ext, mid, resolution, transformation(crs));
    else {
        auto a = cartesian{}(*crs(mid));
        auto b = cartesian{}(*crs(add_meters(mid, resolution, 0.)));
        return affine(ext, {a, b});
    }
}

/// Samples a fixed grid.
geographic::grid geographic_interpolate(
    raster::point ext,
    matrix const& mat,
    srs_spec auto const& crs,
    double meters)
{
    auto inv = transform(forward(mat), backward(transformation(crs)));
    auto x = ext.x() * .5, y = ext.y() * .5;
    auto px = inv(geographic::segment{{x, y}, {x + 1., y + 1.}});
    if (!px)
        return {};
    auto area = 1LL * ext.x() * ext.y();
    auto n = size_t(area * .5 * std::pow(length(*px) / meters, 2)) + 1;
    auto mbr = geographic::box{{}, geographic{}(ext)};
    auto points = inv(box_interpolate(mbr, size_t(2 * std::sqrt(n)) + 1, n));
    if (!points)
        return {};
    auto ret = geographic::grid{};
    for (auto fib : fibonacci_levels) {
        auto step = fib.step();
        if (step < meters * numbers::inv_sqrt_2)
            break;
        auto pred = [&](auto& ll) { return distance(ll, *points) <= step; };
        auto indices = std::unordered_set<size_t>{};
        for (auto& p : *points)
            for (auto i : fib.nearests(p, pred)) {
                if (!indices.insert(i).second)
                    break;
                ret[step].push_back(fib[i]);
            }
    }
    return ret;
}

/// Boxes use crs coordinates; footprint keys are spacings in metres.
/// Skips boxes touching the poles or antimeridian.
auto make_boxes(geographic::grid const& footprint, srs_spec auto const& crs)
{
    auto ret = std::vector<cartesian::box>{};
    auto fwd = transform(forward(transformation(crs)));
    auto add = [&](geographic::box const& ll) {
        auto a = ll.min_corner(), b = ll.max_corner();
        if (a.x() <= -180. || b.x() >= 180. || a.y() <= -90. || b.y() >= 90.)
            return;
        if (auto xy = fwd(ll).transform(cartesian{}))
            ret.push_back(*xy);
    };
    for (auto& lvl : footprint | std::views::reverse) {
        auto d = lvl.first * numbers::inv_sqrt_2;
        if (d >= numbers::earth::sqrt_area / 4)
            continue;
        for (auto& p : lvl.second) {
            auto x = p.x(), y = p.y();
            auto dx = d * meter(y), dy = d * meter();
            add({{x - dx, y - dy}, {x + dx, y + dy}});
        }
    }
    return ret;
}

}  // namespace boat::geometry

#endif  // BOAT_GEOMETRY_RASTER_HPP
