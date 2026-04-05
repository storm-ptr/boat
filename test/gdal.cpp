// Andrew Naplavkov

#include <boat/gdal/command.hpp>
#include <boat/gdal/dal.hpp>
#include <boat/gdal/gil.hpp>
#include <boat/geometry/raster.hpp>
#include <boost/test/unit_test.hpp>
#include "data.hpp"

using namespace boat;

BOOST_AUTO_TEST_CASE(gdal_raster)
{
    namespace bgi = boost::geometry::index;
    namespace gil = boost::gil;

    auto remote = gdal::open(
        "/vsicurl/https://download.osgeo.org/gdal/data/gtiff/small_world.tif");
    auto r = gdal::get_raster(remote.get());
    std::cout << r << std::endl;
    BOOST_CHECK_EQUAL(r.width, 400);
    BOOST_CHECK_EQUAL(r.height, 200);
    auto local = gdal::create("./small_world.tif", "gtiff", r);
    auto copy = [&](int x, int y, int w, int h, auto&& img) {
        gdal::image_io(remote.get(), GF_Read, x, y, w, h, gil::view(img));
        gdal::image_io(local.get(), GF_Write, x, y, w, h, gil::view(img));
    };

    auto tiles =
        std::vector<tile>{{.z = 1, .y = 0, .x = 0}, {.z = 1, .y = 0, .x = 1}};
    for (auto [i, t] : std::views::enumerate(tiles)) {
        auto scale = tile::scale(r.width, r.height, t.z);
        auto [x1, y1] = t.min_corner(r.width, r.height);
        auto [x2, y2] = t.max_corner(r.width, r.height);
        auto w = x2 - x1;
        auto h = y2 - y1;
        if (i % 2)
            copy(x1, y1, w, h, gil::rgb8_image_t{w / scale, h / scale});
        else
            copy(x1, y1, w, h, gil::rgb8_planar_image_t{w / scale, h / scale});
    }

    auto lhs = gil::rgb8_image_t{r.width, r.height};
    gdal::image_io(
        remote.get(), GF_Read, 0, 0, r.width, r.height, gil::view(lhs));
    auto rhs = gil::rgb8_image_t{r.width, r.height};
    gdal::image_io(
        local.get(), GF_Read, 0, 0, r.width, r.height, gil::view(rhs));
    BOOST_CHECK(lhs == rhs);
}

BOOST_AUTO_TEST_CASE(gdal_vector)
{
    auto dal = gdal::dal{};
    dal.dataset = gdal::create("", "mem");
    check(dal);
}
