// Andrew Naplavkov

#ifndef BOAT_TEST_GUI_CONTEXT_HPP
#define BOAT_TEST_GUI_CONTEXT_HPP

#include <boat/geometry/raster.hpp>
#include <boat/tile.hpp>
#include "../utility.hpp"

constexpr auto transparent_ratio = .0935;
constexpr auto transparent_tolerance = .002;

struct context {
    boat::geometry::raster::point extent;
    boat::geometry::geographic::grid footprint;
    boat::geometry::matrix affine;
    boat::geometry::srs_variant crs;
};

inline std::generator<context> contexts()
{
    using namespace boat::geometry;
    auto ext = raster::point{1280, 720};
    for (auto p : {geographic::point{20., 40.}, {-117.5, 33.7}, {-179., 68.}})
        for (auto res : {100., 10'000.})
            for (auto deg : {0., 23.5})
                for (auto crs : {lonlat, ortho(p)}) {
                    auto fwd = transform(forward(transformation(crs)));
                    auto rad = deg * boat::numbers::degree;
                    auto a = *fwd(p);
                    auto b = *fwd(add_meters(
                        p, res * std::cos(rad), res * std::sin(rad)));
                    auto pixel =
                        cartesian::segment{{a.x(), a.y()}, {b.x(), b.y()}};
                    auto mat = affine(ext, pixel);
                    auto footprint = geographic_interpolate(
                        ext, mat, crs, res * boat::tile::half_size);
                    co_yield {ext, footprint, mat, crs};
                }
}

#endif  // BOAT_TEST_GUI_CONTEXT_HPP
