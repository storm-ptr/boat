// Andrew Naplavkov

#ifndef BOAT_GUI_VARIANT_HPP
#define BOAT_GUI_VARIANT_HPP

#include <boat/geometry/transform.hpp>
#include <boost/gil.hpp>

namespace boat::gui {

struct raster {
    boost::gil::rgba8_image_t rgba;
    geometry::matrix affine;
    geometry::srs_variant crs;
};

using variant = std::variant<geometry::geographic::geometry_collection, raster>;

auto draw_variant(  //
    execution_policy auto policy,
    auto& out,
    geometry::matrix const& out_affine,
    geometry::srs_variant const& out_crs)
{
    return overloaded{
        [=, &out](geometry::geographic::geometry_collection const& in) {
            auto fwd = geometry::transform(
                geometry::srs_forward(geometry::transformation(out_crs)),
                geometry::mat_inverse(out_affine));
            auto drw = draw_geometry(out);
            if (auto g = fwd(in))
                drw(*g);
        },
        [=, &out](raster const& in) {
            draw_image(  //
                policy,
                const_view(in.rgba),
                in.affine,
                in.crs,
                out,
                out_affine,
                out_crs);
        }};
}

}  // namespace boat::gui

#endif  // BOAT_GUI_VARIANT_HPP
