// Andrew Naplavkov

#include <boat/geometry/map.hpp>
#include <boat/slippy/geometry.hpp>
#include <boost/geometry/srs/epsg.hpp>
#include <boost/test/unit_test.hpp>

BOOST_AUTO_TEST_CASE(slippy)
{
    auto ll = boat::geometry::geographic::point{139.7006793, 35.6590699};
    auto t = boat::slippy::to_tile(ll, 18);
    BOOST_CHECK_EQUAL(t.x, 232'798);
    BOOST_CHECK_EQUAL(t.y, 103'246);
    BOOST_CHECK(boost::geometry::within(ll, envelope(t)));

    auto pj = boost::geometry::srs::projection<
        boost::geometry::srs::static_epsg<3857>>{};
    auto xy = boat::geometry::cartesian::point{};
    BOOST_CHECK(pj.forward(ll, xy));
    auto resolution = 611.5;
    auto scale = boat::geometry::scale(pj, ll, resolution);
    auto width = 720;
    auto height = 480;
    auto mbr = boat::geometry::envelope(xy, scale, width, height);
    auto num_points = width * height / (128 * 128);
    auto grid = boat::geometry::inverse(pj, mbr, num_points);
    auto tiles = boat::slippy::to_tiles(grid, resolution);

    auto z = 8;
    auto xmin = INT_MAX, xmax = INT_MIN, ymin = INT_MAX, ymax = INT_MIN;
    for (auto xy : boost::geometry::box_view{mbr} | std::views::take(4)) {
        BOOST_CHECK(pj.inverse(xy, ll));
        t = boat::slippy::to_tile(ll, z);
        xmin = std::min<>(xmin, t.x);
        xmax = std::max<>(xmax, t.x);
        ymin = std::min<>(ymin, t.y);
        ymax = std::max<>(ymax, t.y);
    }
    for (int x = xmin; x <= xmax; ++x)
        for (int y = ymin; y <= ymax; ++y)
            BOOST_CHECK(tiles.contains({x, y, z}));
}
