// Andrew Naplavkov

#include <boat/gdal/make_image.hpp>
#include <boat/gdal/raster.hpp>
#include <boat/geometry/raster.hpp>
#include <boat/geometry/tile.hpp>
#include <boost/test/unit_test.hpp>

BOOST_AUTO_TEST_CASE(gdal)
{
    namespace bgi = boost::geometry::index;
    auto remote = boat::gdal::raster{
        "/vsicurl/https://download.osgeo.org/gdal/data/gtiff/small_world.tif"};
    auto m = remote.get_meta();
    m.file = "./small_world.tif";
    std::cout << m << std::endl;
    auto local = boat::gdal::raster{"GTiff", m};
    auto srs = boost::geometry::srs::epsg{m.epsg};
    auto num_points = 20;
    auto grid = boat::geometry::geographic_interpolate(
        m.width, m.height, m.affine, srs, num_points);
    auto res = 300;
    auto tiles =
        boat::geometry::covers(m.width, m.height, m.affine, srs, grid, res);
    BOOST_CHECK_EQUAL(tiles.size(), 2);
    auto rtree = bgi::rtree<boat::geometry::cartesian::box, bgi::rstar<4>>{};
    for (auto [i, t] : std::views::enumerate(tiles) | std::views::take(2)) {
        auto mbr = boat::geometry::envelope(m.width, m.height, t);
        BOOST_CHECK(rtree.qbegin(bgi::overlaps(mbr)) == rtree.qend());
        rtree.insert(mbr);
        auto a = mbr.min_corner(), b = mbr.max_corner();
        auto x = static_cast<int>(a.x());
        auto y = static_cast<int>(a.y());
        auto w = static_cast<int>(b.x() - a.x());
        auto h = static_cast<int>(b.y() - a.y());
        auto scale = boat::geometry::downscaling_factor(m.width, m.height, t.z);
        auto img = boat::gdal::make_image(
            w / scale,
            h / scale,
            m.bands | std::views::transform(&boat::gdal::band::color));
        boost::variant2::visit(
            [&](auto& v) {
                remote.io<GF_Read>(x, y, w, h, boost::gil::view(v));
                local.io<GF_Write>(x, y, w, h, boost::gil::view(v));
            },
            img);
    }
    for (auto p : boat::geometry::box_interpolate<
             boat::geometry::cartesian::multi_point>(
             m.width, m.height, num_points)) {
        BOOST_CHECK(rtree.qbegin(bgi::intersects(p)) != rtree.qend());
    }
}
