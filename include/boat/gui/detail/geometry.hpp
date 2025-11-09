// Andrew Naplavkov

#ifndef BOAT_GUI_GEOMETRY_HPP
#define BOAT_GUI_GEOMETRY_HPP

#include <boat/geometry/distribution.hpp>
#include <boat/geometry/transform.hpp>
#include <boat/geometry/wkb.hpp>

namespace boat::gui {

inline auto multi_point(int width, int height)
{
    auto ret = geometry::geographic::multi_point{};
    auto mbr = geometry::geographic::box{{}, {width * 1., height * 1.}};
    ret.append_range(geometry::box_interpolate(mbr, 7));
    ret.append_range(geometry::box_fibonacci(mbr, 37));
    return ret;
}

inline auto epsg(int code)
{
    return boost::geometry::srs::epsg{code};
}

auto forward(geometry::srs_spec auto const& srs1,
             geometry::matrix const& affine2,
             geometry::srs_spec auto const& srs2)
{
    auto tf = boost::geometry::srs::transformation<>{srs1, srs2};
    return geometry::transform(geometry::srs_forward(tf),
                               geometry::mat_inverse(affine2));
}

auto bidirectional(geometry::matrix const& affine1,
                   geometry::srs_spec auto const& srs1,
                   geometry::matrix const& affine2,
                   geometry::srs_spec auto const& srs2)
{
    auto tf = boost::geometry::srs::transformation<>{srs1, srs2};
    return std::pair{geometry::transform(geometry::mat_forward(affine1),
                                         geometry::srs_forward(tf),
                                         geometry::mat_inverse(affine2)),
                     geometry::transform(geometry::mat_forward(affine2),
                                         geometry::srs_inverse(tf),
                                         geometry::mat_inverse(affine1))};
}

inline auto variant(blob_view wkb)
{
    auto ret = geometry::geographic::variant{};
    wkb >> ret;
    return ret;
}

}  // namespace boat::gui

#endif  // BOAT_GUI_GEOMETRY_HPP
