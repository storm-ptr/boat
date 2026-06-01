// Andrew Naplavkov

#ifndef BOAT_GUI_VARIANT_HPP
#define BOAT_GUI_VARIANT_HPP

#include <boat/geometry/transform.hpp>
#include <boat/geometry/wkb.hpp>

namespace boat::gui {

struct vector {
    std::vector<blob> wkb;
    geometry::srs_variant crs;
};

template <class T = blob>
struct raster {
    T data;
    geometry::matrix affine;
    geometry::srs_variant crs;
};

using variant = std::variant<vector, raster<>>;

template <class>
struct traits;

template <class T, class Traits = typename traits<T>::type>
auto draw_variant(  //
    T& art,
    geometry::matrix const& affine,
    geometry::srs_variant const& crs)
{
    return overloaded{
        [&](vector const& in) {
            auto fwd = std::visit(
                [&](auto& crs1, auto& crs2) {
                    return geometry::transform(
                        geometry::srs_forward(
                            geometry::srs::transformation<>(crs1, crs2)),
                        geometry::mat_inverse(affine));
                },
                in.crs,
                crs);
            auto drw = Traits::draw_geometry(art);
            for (blob_view item : in.wkb) {
                auto g1 = geometry::geographic::variant{};
                item >> g1;
                if (auto g2 = fwd(g1))
                    drw(*g2);
            }
        },
        [&](raster<typename Traits::image> const& in) {
            std::visit(
                [&](auto& crs1, auto& crs2) {
                    Traits::draw_image(
                        in.data, in.affine, crs1, art, affine, crs2);
                },
                in.crs,
                crs);
        },
        [](this auto&& self, raster<> const& in) -> void {
            auto tmp = raster<typename Traits::image>{{}, in.affine, in.crs};
            if (Traits::load_image(in.data, tmp.data))
                self(tmp);
        }};
}

}  // namespace boat::gui

#endif  // BOAT_GUI_VARIANT_HPP
