// Andrew Naplavkov

#ifndef BOAT_GUI_GEOMETRY_HPP
#define BOAT_GUI_GEOMETRY_HPP

#include <boat/geometry/transform.hpp>
#include <boat/geometry/wkb.hpp>

namespace boat::gui {

constexpr geometry::geographic::box box(double width, double height)
{
    return {{0., 0.}, {width, height}};
}

inline auto epsg(int code)
{
    return boost::geometry::srs::epsg{code};
}

auto forward(geometry::srs_params auto const& srs1,
             geometry::srs_params auto const& srs2,
             boost::qvm::mat<double, 3, 3> const& affine2)
{
    auto tf = boost::geometry::srs::transformation<>{srs1, srs2};
    return geometry::transform(geometry::srs_forward(tf),
                               geometry::matrix(boost::qvm::inverse(affine2)));
}

auto bidirectional(boost::qvm::mat<double, 3, 3> const& affine1,
                   geometry::srs_params auto const& srs1,
                   geometry::srs_params auto const& srs2,
                   boost::qvm::mat<double, 3, 3> const& affine2)
{
    auto tf = boost::geometry::srs::transformation<>{srs1, srs2};
    return std::pair{
        geometry::transform(geometry::matrix(affine1),
                            geometry::srs_forward(tf),
                            geometry::matrix(boost::qvm::inverse(affine2))),
        geometry::transform(geometry::matrix(affine2),
                            geometry::srs_inverse(tf),
                            geometry::matrix(boost::qvm::inverse(affine1)))};
}

inline auto variant(blob_view wkb)
{
    auto ret = geometry::geographic::variant{};
    wkb >> ret;
    return ret;
}

}  // namespace boat::gui

#endif  // BOAT_GUI_GEOMETRY_HPP
