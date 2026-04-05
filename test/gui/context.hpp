// Andrew Naplavkov

#ifndef BOAT_TEST_GUI_CONTEXT_HPP
#define BOAT_TEST_GUI_CONTEXT_HPP

#include <boat/geometry/raster.hpp>
#include <boat/tile.hpp>
#include "../utility.hpp"

constexpr auto transparent_ratio = .0935;
constexpr auto transparent_tolerance = .002;

struct context {
    int width;
    int height;
    boat::geometry::geographic::grid grid;
    boat::geometry::matrix affine;
    boost::geometry::srs::proj4 system;
};

inline std::generator<context> contexts()
{
    using namespace boat::geometry;
    auto width = 1280;
    auto height = 720;
    auto num_points = (width * height) / (boat::tile::size * boat::tile::size);
    for (auto p : {geographic::point{20., 40.}, {-117.5, 33.7}, {-179., 68.}})
        for (auto res : {100., 10'000.})
            for (auto deg : {0., 23.5})
                for (auto sys : {lonlat, ortho(p)}) {
                    auto fwd = transform(srs_forward(transformation(sys)));
                    auto rad = deg * boat::numbers::degree;
                    auto a = *fwd(p);
                    auto b = *fwd(add_meters(
                        p, res * std::cos(rad), res * std::sin(rad)));
                    auto pixel =
                        cartesian::segment{{a.x(), a.y()}, {b.x(), b.y()}};
                    auto mat = affine(width, height, pixel);
                    auto grid = geographic_interpolate(
                        width, height, mat, sys, num_points);
                    co_yield {width, height, grid, mat, sys};
                }
}

#endif  // BOAT_TEST_GUI_CONTEXT_HPP
