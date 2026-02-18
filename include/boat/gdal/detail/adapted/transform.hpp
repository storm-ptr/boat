// Andrew Naplavkov

#ifndef BOAT_GDAL_ADAPTED_TRANSFORM_HPP
#define BOAT_GDAL_ADAPTED_TRANSFORM_HPP

#include <boat/gdal/detail/utility.hpp>
#include <boat/geometry/vocabulary.hpp>

namespace boat::gdal {

inline geometry::matrix get_transform(GDALDatasetH ds)
{
    auto a = std::array<double, 6>{};
    check(GDALGetGeoTransform(ds, a.data()));
    return {{{a[1], a[2], a[0]}, {a[4], a[5], a[3]}, {0., 0., 1.}}};
}

inline void set_transform(GDALDatasetH ds, geometry::matrix const& mat)
{
    auto& m = mat.a;
    auto a = std::array{m[0][2], m[0][0], m[0][1], m[1][2], m[1][0], m[1][1]};
    check(GDALSetGeoTransform(ds, a.data()));
}

}  // namespace boat::gdal

#endif  // BOAT_GDAL_ADAPTED_TRANSFORM_HPP
