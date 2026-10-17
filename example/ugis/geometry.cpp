// Andrew Naplavkov

#include "geometry.h"

boat::geometry::matrix affine(  //
    int width,
    int height,
    boat::gui::view const& view,
    boat::geometry::srs::proj4 const& crs)
{
    auto tf = boat::geometry::transformation(crs);
    return affine(width, height, view, tf);
}

boat::geometry::matrix affine(  //
    int width,
    int height,
    boat::gui::view const& view,
    boat::geometry::srs::transformation<> const& tf)
{
    auto fwd = boat::geometry::transform(boat::geometry::srs_forward(tf));
    return affine(width, height, view, fwd);
}
