// Andrew Naplavkov

#include <boat/gdal/command.hpp>
#include <boat/gdal/gil.hpp>
#include <boat/gdal/raster.hpp>
#include <boat/gdal/vector.hpp>
#include <boat/geometry/raster.hpp>
#include <boat/geometry/tile.hpp>
#include <boat/sql/io.hpp>
#include <boost/test/unit_test.hpp>
#include "data.hpp"

using namespace boat;

BOOST_AUTO_TEST_CASE(gdal_raster)
{
    namespace bgi = boost::geometry::index;
    namespace gil = boost::gil;

    auto remote = gdal::open(
        "/vsicurl/https://download.osgeo.org/gdal/data/gtiff/small_world.tif");
    auto r = *gdal::describe(remote.get());
    std::cout << r << std::endl;
    auto local = gdal::create("./small_world.tif", "gtiff", r);
    r = *gdal::describe(local.get());
    std::cout << r << std::endl;
    auto copy = [&](int x, int y, int w, int h, auto&& img) {
        gdal::image_io(remote.get(), GF_Read, x, y, w, h, gil::view(img));
        gdal::image_io(local.get(), GF_Write, x, y, w, h, gil::view(img));
    };

    auto srs = boost::geometry::srs::epsg{r.epsg};
    auto num_points = 20;
    auto grid = geometry::geographic_interpolate(
        r.width, r.height, r.affine, srs, num_points);
    auto tiles = geometry::covers(  //
        grid,
        300,  //< resulution
        512,  //< limit
        r.width,
        r.height,
        r.affine,
        srs);
    BOOST_CHECK_EQUAL(tiles.size(), 2);
    auto rtree = bgi::rtree<geometry::cartesian::box, bgi::rstar<4>>{};
    for (auto [i, t] : std::views::enumerate(tiles) | std::views::take(2)) {
        auto mbr = geometry::envelope(r.width, r.height, t);
        BOOST_CHECK(rtree.qbegin(bgi::overlaps(mbr)) == rtree.qend());
        rtree.insert(mbr);
        auto a = mbr.min_corner();
        auto b = mbr.max_corner();
        auto x = static_cast<int>(a.x());
        auto y = static_cast<int>(a.y());
        auto w = static_cast<int>(b.x() - a.x());
        auto h = static_cast<int>(b.y() - a.y());
        auto scale = geometry::downscaling(r.width, r.height, t.z);
        if (i % 2)
            copy(x, y, w, h, gil::rgb8_image_t{w / scale, h / scale});
        else
            copy(x, y, w, h, gil::rgb8_planar_image_t{w / scale, h / scale});
    }
    for (auto p : geometry::box_interpolate<geometry::cartesian::multi_point>(
             r.width, r.height, num_points)) {
        BOOST_CHECK(rtree.qbegin(bgi::intersects(p)) != rtree.qend());
    }
}

BOOST_AUTO_TEST_CASE(gdal_vector)
{
    auto objs = get_objects();
    auto page = sql::page{
        .select_list = boost::pfr::names_as_array<object_struct>() |
                       std::ranges::to<std::vector<std::string>>(),
        .limit = static_cast<int>(std::ranges::size(objs)),
    };
    auto bbox = sql::bbox{
        .select_list = {std::string(boost::pfr::get_name<0, object_struct>())},
        .xmin = 9,
        .ymin = 9,
        .xmax = 11,
        .ymax = 11,
        .limit = int(std::ranges::size(objs))};
    auto draft = get_object_table();
    auto ds = gdal::create("", "mem");
    auto tbl = gdal::create(ds.get(), draft);
    std::cout << tbl;
    auto rows = pfr::to_rowset(objs);
    gdal::insert(ds.get(), tbl, rows);
    BOOST_CHECK(std::ranges::equal(
        objs,
        gdal::select(ds.get(), tbl, page) | pfr::view<object_struct>,
        BOAT_LIFT(boost::pfr::eq_fields)));
    BOOST_CHECK(std::ranges::equal(
        std::array{2}, gdal::select(ds.get(), tbl, bbox) | pfr::view<int>));
}
