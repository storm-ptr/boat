// Andrew Naplavkov

#include "geometry.h"

boat::geometry::matrix affine(  //
    int width,
    int height,
    boat::geometry::geographic::point const& mid,
    double resolution,
    boat::geometry::srs::proj4 const& crs)
{
    auto tf = boat::geometry::transformation(crs);
    return affine(width, height, mid, resolution, tf);
}

boat::geometry::matrix affine(  //
    int width,
    int height,
    boat::geometry::geographic::point const& mid,
    double resolution,
    boat::geometry::srs::transformation<> const& tf)
{
    auto fwd = boat::geometry::transform(boat::geometry::srs_forward(tf));
    return affine(width, height, mid, resolution, fwd);
}
