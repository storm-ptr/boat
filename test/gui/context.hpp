// Andrew Naplavkov

#ifndef BOAT_TEST_GUI_CONTEXT_HPP
#define BOAT_TEST_GUI_CONTEXT_HPP

#include <boat/geometry/raster.hpp>
#include <generator>

constexpr auto transparent_ratio = .227;
constexpr auto transparent_tolerance = .002;

struct context {
    int width;
    int height;
    boat::geometry::geographic::grid grid;
    double resolution;
    boat::geometry::matrix affine;
    boost::geometry::srs::proj4 srs;
};

inline std::generator<context> contexts()
{
    using namespace boat::geometry;
    for (auto p : {geographic::point{25., 25.}, {-179., 68.}})
        for (auto [width, height] : {std::pair{1280, 720}, {1920, 1080}})
            for (auto res : {500., 10'000.})
                for (auto deg : {0., 23.5})
                    for (auto srs : {srs_lonlat, srs_ortho(p)}) {
                        auto fwd = transform(srs_forward(transformation(srs)));
                        auto rad = deg * boat::numbers::deg;
                        auto a = *fwd(p);
                        auto b = *fwd(add_meters(
                            p, res * std::cos(rad), res * std::sin(rad)));
                        auto pixel =
                            cartesian::segment{{a.x(), a.y()}, {b.x(), b.y()}};
                        auto mat = to_matrix(width, height, pixel);
                        auto num_points = (width * height) / (96 * 96);
                        auto grid =
                            tessellation(width, height, num_points, mat, srs);
                        co_yield {width, height, grid, res, mat, srs};
                    }
}

#endif  // BOAT_TEST_GUI_CONTEXT_HPP
