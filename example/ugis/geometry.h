// Andrew Naplavkov

#ifndef GEOMETRY_H
#define GEOMETRY_H

#include <boat/geometry/raster.hpp>

boat::geometry::matrix affine(  //
    int width,
    int height,
    boat::geometry::geographic::point const& mid,
    double scale,
    boat::geometry::srs::proj4 const&);

boat::geometry::matrix affine(  //
    int width,
    int height,
    boat::geometry::geographic::point const& mid,
    double scale,
    boat::geometry::srs::transformation<> const&);

boat::geometry::matrix affine(  //
    int width,
    int height,
    boat::geometry::geographic::point const& mid,
    double scale,
    auto const& fwd)
{
    auto a = *fwd(mid), b = *fwd(boat::geometry::add_meters(mid, scale, 0.));
    auto px =
        boat::geometry::cartesian::segment{{a.x(), a.y()}, {b.x(), b.y()}};
    return boat::geometry::affine(width, height, px);
}

#endif  // GEOMETRY_H
