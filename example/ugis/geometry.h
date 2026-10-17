// Andrew Naplavkov

#ifndef GEOMETRY_H
#define GEOMETRY_H

#include <boat/geometry/raster.hpp>
#include <boat/gui/scene.hpp>

boat::geometry::matrix affine(  //
    int width,
    int height,
    boat::gui::view const&,
    boat::geometry::srs::proj4 const&);

boat::geometry::matrix affine(  //
    int width,
    int height,
    boat::gui::view const&,
    boat::geometry::srs::transformation<> const&);

boat::geometry::matrix affine(  //
    int width,
    int height,
    boat::gui::view const& view,
    auto const& fwd)
{
    auto a = *fwd(view.center);
    auto b = *fwd(boat::geometry::add_meters(view.center, view.resolution, 0.));
    auto px =
        boat::geometry::cartesian::segment{{a.x(), a.y()}, {b.x(), b.y()}};
    return boat::geometry::affine(width, height, px);
}

#endif  // GEOMETRY_H
