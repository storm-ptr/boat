// Andrew Naplavkov

#include <boat/gdal/raster.hpp>
#include <boat/geometry/raster.hpp>
#include <boat/geometry/tile.hpp>
#include <boost/test/unit_test.hpp>

BOOST_AUTO_TEST_CASE(gdal)
{
    namespace bgi = boost::geometry::index;
    namespace gil = boost::gil;

    auto remote = boat::gdal::raster{
        "/vsicurl/https://download.osgeo.org/gdal/data/gtiff/small_world.tif"};
    auto m = remote.get_meta();
    std::cout << m << std::endl;
    m.file = "./small_world.tif";
    auto local = boat::gdal::raster{"GTiff", m};
    auto copy = [&](int x, int y, int w, int h, auto&& img) {
        remote.io<GF_Read>(x, y, w, h, gil::view(img));
        local.io<GF_Write>(x, y, w, h, gil::view(img));
    };

    auto srs = boost::geometry::srs::epsg{m.epsg};
    auto num_points = 20;
    auto grid = boat::geometry::geographic_interpolate(
        m.width, m.height, m.affine, srs, num_points);
    auto tiles = boat::geometry::covers(  //
        grid,
        300,  //< resulution
        512,  //< limit
        m.width,
        m.height,
        m.affine,
        srs);
    BOOST_CHECK_EQUAL(tiles.size(), 2);
    auto rtree = bgi::rtree<boat::geometry::cartesian::box, bgi::rstar<4>>{};
    for (auto [i, t] : std::views::enumerate(tiles) | std::views::take(2)) {
        auto mbr = boat::geometry::envelope(m.width, m.height, t);
        BOOST_CHECK(rtree.qbegin(bgi::overlaps(mbr)) == rtree.qend());
        rtree.insert(mbr);
        auto a = mbr.min_corner();
        auto b = mbr.max_corner();
        auto x = static_cast<int>(a.x());
        auto y = static_cast<int>(a.y());
        auto w = static_cast<int>(b.x() - a.x());
        auto h = static_cast<int>(b.y() - a.y());
        auto scale = boat::geometry::downscaling_factor(m.width, m.height, t.z);
        if (i % 2)
            copy(x, y, w, h, gil::rgb8_image_t{w / scale, h / scale});
        else
            copy(x, y, w, h, gil::rgb8_planar_image_t{w / scale, h / scale});
    }
    for (auto p : boat::geometry::box_interpolate<
             boat::geometry::cartesian::multi_point>(
             m.width, m.height, num_points)) {
        BOOST_CHECK(rtree.qbegin(bgi::intersects(p)) != rtree.qend());
    }
}
