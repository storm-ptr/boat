// Andrew Naplavkov

#include <boat/gdal/catalog.hpp>
#include <boat/gdal/command.hpp>
#include <boat/gdal/detail/image_io.hpp>
#include <boat/geometry/raster.hpp>
#include <boat/slippy.hpp>
#include <boost/test/unit_test.hpp>
#include "data.hpp"

BOOST_AUTO_TEST_CASE(gdal_source)
{
    auto cat = boat::gdal::catalog{};
    cat.dataset = boat::gdal::open(
        R"(WMS:https://wms.gebco.net/mapserv?request=GetCapabilities&service=WMS)");
    auto sources = cat.sources();
    BOOST_CHECK(!sources.empty());
    for (auto& src : sources | std::views::take(1)) {
        std::cout << src.source_name << ": " << src.address << std::endl;
        auto sub = boat::gdal::catalog{};
        sub.dataset = boat::gdal::open(src.address.data());
        auto lyrs = sub.layers();
        BOOST_CHECK(!lyrs.empty());
        for (auto& lyr : lyrs) {
            BOOST_CHECK(lyr.raster);
            std::cout << sub.get_raster(lyr) << std::endl;
        }
    }
}

BOOST_AUTO_TEST_CASE(gdal_vector)
{
    auto cat = boat::gdal::catalog{};
    cat.dataset = boat::gdal::create("", "mem");
    check(cat);
}

BOOST_AUTO_TEST_CASE(gdal_raster)
{
    auto cat1 = boat::slippy::catalog{};
    cat1.user = "useragent";
    cat1.url = "http://mt.google.com/vt/lyrs=s&z={z}&x={x}&y={y}";
    cat1.zmax = 2;
    auto rast1 = cat1.get_raster(cat1.layers().at(0));

    auto cat2 = boat::gdal::catalog{};
    cat2.dataset = boat::gdal::create("./gdal_raster.tif", "gtiff", rast1);
    auto rast2 = cat2.get_raster(cat2.layers().at(0));

    auto z = boat::tile::zmax(rast1.width, rast1.height);
    auto tiles = boat::tile::all(rast1.width, rast1.height, z) |
                 std::views::take(16) | std::ranges::to<std::vector>();
    BOOST_CHECK(!tiles.empty());
    auto img1 = cat1.read(rast1, tiles) | std::ranges::to<std::map>();
    BOOST_CHECK_EQUAL(img1.size(), tiles.size());
    for (auto [tile, img] : img1)
        cat2.write(  //
            rast2,
            std::make_from_tuple<boat::db::rect>(
                tile.rect(rast2.width, rast2.height)),
            const_view(img));
    auto img2 = cat2.read(rast2, tiles) | std::ranges::to<std::map>();
    BOOST_CHECK_EQUAL(img2.size(), tiles.size());
    for (auto& tile : tiles)
        BOOST_CHECK(img1.at(tile) == img2.at(tile));
}

BOOST_AUTO_TEST_CASE(gdal_image_io)
{
    namespace gil = boost::gil;
    auto ds1 = boat::gdal::open(
        "/vsicurl/https://download.osgeo.org/gdal/data/gtiff/small_world.tif");
    auto rast = boat::gdal::get_raster(ds1.get());
    BOOST_CHECK_EQUAL(rast.width, 400);
    BOOST_CHECK_EQUAL(rast.height, 200);
    auto ds2 = boat::gdal::create("./gdal_image_io.tif", "gtiff", rast);
    auto copy = [&](int x, int y, int w, int h, auto&& img) {
        boat::gdal::image_io(ds1.get(), GF_Read, x, y, w, h, gil::view(img));
        boat::gdal::image_io(ds2.get(), GF_Write, x, y, w, h, gil::view(img));
    };
    auto tiles = std::vector<boat::tile>{
        {.z = 1, .y = 0, .x = 0},
        {.z = 1, .y = 0, .x = 1},
    };
    for (auto [i, tile] : std::views::enumerate(tiles)) {
        auto scale = boat::tile::scale(rast.width, rast.height, tile.z);
        auto [x, y, w, h] = tile.rect(rast.width, rast.height);
        if (i % 2)
            copy(x, y, w, h, gil::rgb8_image_t{w / scale, h / scale});
        else
            copy(x, y, w, h, gil::rgb8_planar_image_t{w / scale, h / scale});
    }
    auto img1 = gil::rgb8_image_t{rast.width, rast.height};
    boat::gdal::image_io(
        ds1.get(), GF_Read, 0, 0, rast.width, rast.height, gil::view(img1));
    auto img2 = gil::rgb8_image_t{rast.width, rast.height};
    boat::gdal::image_io(
        ds2.get(), GF_Read, 0, 0, rast.width, rast.height, gil::view(img2));
    BOOST_CHECK(img1 == img2);
}
