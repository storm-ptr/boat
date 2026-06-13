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

template <class T>
auto draw_variant(  //
    T& out,
    geometry::matrix const& out_affine,
    geometry::srs_variant const& out_crs)
{
    return overloaded{
        [&](geometry::geographic::geometry_collection const& in) {
            auto fwd = std::visit(
                [&](auto& crs) {
                    return geometry::transform(
                        geometry::srs_forward(geometry::transformation(crs)),
                        geometry::mat_inverse(out_affine));
                },
                out_crs);
            auto drw = draw_geometry(out);
            if (auto g = fwd(in))
                drw(*g);
        },
        [&](raster const& in) {
            std::visit(
                [&](auto& crs1, auto& crs2) {
                    draw_image(  //
                        const_view(in.rgba),
                        in.affine,
                        crs1,
                        out,
                        out_affine,
                        crs2);
                },
                in.crs,
                out_crs);
        }};
}

}  // namespace boat::gui

#endif  // BOAT_GUI_VARIANT_HPP
