// Andrew Naplavkov

#ifndef BOAT_GUI_VARIANT_HPP
#define BOAT_GUI_VARIANT_HPP

#include <boat/geometry/transform.hpp>
#include <boost/gil.hpp>

namespace boat::gui {

struct vector {
    geometry::geographic::geometry_collection geoms;
    bool partial = false;  ///< Query limit reached.
};

struct raster {
    boost::gil::rgba8_image_t rgba;
    geometry::matrix affine;  ///< Maps pixels to spatial coordinates.
    geometry::srs_variant crs;
};

using variant = std::variant<vector, raster>;

auto draw_variant(
    execution_policy auto policy,
    auto& out,
    geometry::matrix const& out_affine,
    geometry::srs_variant const& out_crs)
{
    struct adl_ref {
        decltype(out) value;
        operator decltype(out)() const { return value; }
    };
    auto ref = adl_ref{out};
    return overloaded{
        [=](vector const& in) {
            auto fwd = geometry::transform(
                geometry::srs_forward(geometry::transformation(out_crs)),
                geometry::mat_inverse(out_affine));
            if (auto g = fwd(in.geoms))
                draw_geometry(ref)(*g);
        },
        [=](raster const& in) {
            draw_image(
                policy,
                const_view(in.rgba),
                in.affine,
                in.crs,
                ref,
                out_affine,
                out_crs);
        }};
}

}  // namespace boat::gui

#endif  // BOAT_GUI_VARIANT_HPP
