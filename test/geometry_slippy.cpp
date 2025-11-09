// Andrew Naplavkov

#include <boat/geometry/raster.hpp>
#include <boat/geometry/slippy.hpp>
#include <boost/test/unit_test.hpp>

using namespace boat::geometry;
using namespace boost::geometry;

BOOST_AUTO_TEST_CASE(geometry_slippy_to_tile)
{
    auto p = geographic::point{139.7006793, 35.6590699};
    auto t = slippy::to_tile(p, 18);
    BOOST_CHECK_EQUAL(t.x, 232'798);
    BOOST_CHECK_EQUAL(t.y, 103'246);
    BOOST_CHECK(covered_by(p, slippy::minmax(t)));
}

BOOST_AUTO_TEST_CASE(geometry_slippy_coverage)
{
    auto width = 720;
    auto height = 480;
    auto num_points = 21;
    auto p = geographic::point{-0.158526, 51.523757};
    auto res = 611.5;
    auto z = 8;
    auto srs = srs::epsg{slippy::epsg};
    auto tf = transformation(srs);
    auto pixel = cartesian::segment{};
    tf.forward(geographic::segment{p, add_meters(p, 0, res)}, pixel);
    auto mat = to_matrix(width, height, pixel);
    auto grid = tessellation(width, height, num_points, mat, srs);
    auto tiles = slippy::coverage(grid, res);
    auto inv = transform(mat_forward(mat), srs_inverse(tf));
    auto a = slippy::to_tile(*inv(geographic::point()), z);
    auto b = slippy::to_tile(*inv(geographic::point(width, height)), z);
    for (int x = std::min<>(a.x, b.x); x <= std::max<>(a.x, b.x); ++x)
        for (int y = std::min<>(a.y, b.y); y <= std::max<>(a.y, b.y); ++y)
            BOOST_CHECK(tiles.contains({x, y, z}));
}
