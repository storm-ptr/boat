// Andrew Naplavkov

#include <boat/detail/address.hpp>
#include <boat/gdal/catalog.hpp>
#include <boat/gdal/command.hpp>
#include <boat/gdal/detail/image_io.hpp>
#include <boat/geometry/raster.hpp>
#include <boat/slippy.hpp>
#include <boost/test/unit_test.hpp>
#include "data.hpp"

BOOST_AUTO_TEST_CASE(gdal_date_time)
{
    namespace sc = std::chrono;
    auto fd = boat::unique_ptr<void, OGR_FD_Release>{OGR_FD_Create("")};
    OGR_FD_Reference(fd.get());
    auto fld = boat::unique_ptr<void, OGR_Fld_Destroy>{
        OGR_Fld_Create("timestamp", OFTDateTime)};
    OGR_FD_AddFieldDefn(fd.get(), fld.get());
    auto feat = boat::gdal::feature_ptr{OGR_F_Create(fd.get())};
    OGR_F_SetFieldDateTimeEx(
        feat.get(), 0, 2004, 10, 19, 13, 23, 54, OGR_TZFLAG_UTC + 12);
    auto expected = boat::gdal::time_point{
        sc::sys_days{sc::year{2004} / 10 / 19}.time_since_epoch() +
        sc::hours{10} + sc::minutes{23} + sc::seconds{54}};
    BOOST_CHECK(boat::gdal::get_date_time(feat.get(), 0) == expected);
}

BOOST_AUTO_TEST_CASE(gdal_source)
{
    auto cat = boat::gdal::catalog{};
    cat.dataset = boat::gdal::open(
        R"(wms:https://gibs.earthdata.nasa.gov/twms/epsg4326/best/twms.cgi?request=GetTileService)");
    auto sources = cat.sources();
    BOOST_CHECK(!sources.empty());
    for (auto& src : sources | std::views::take(2)) {
        std::cout << src.source_name << "\n";
        auto sub = boat::gdal::catalog{};
        sub.dataset = boat::gdal::open(src.address.data());
        auto lyrs = sub.layers();
        BOOST_CHECK(!lyrs.empty());
        for (auto& lyr : lyrs) {
            BOOST_CHECK(lyr.raster);
            std::cout << sub.get_raster(lyr) << "\n";
        }
    }
}

BOOST_AUTO_TEST_CASE(gdal_vector)
{
    struct {
        std::string path;
        std::string driver;
    } tests[] = {
        {"", "MEM"},
        {"./drop.gdal_vector.gpkg", "GPKG"},
        {"./drop.gdal_vector.sqlite", "SQLite"},
        {boat::config::mssql_gdal_address(), ""},
        {boat::config::postgres_gdal_address, ""},

        // OFTDateTime is created without ms
        // {boat::config::mysql_gdal_address, ""},
    };
    for (auto [path, driver] : tests) {
        if (path.empty() && driver.empty())
            continue;
        auto cat = boat::gdal::catalog{};
        if (driver.empty())
            cat.dataset = boat::gdal::open(path.data(), true);
        else
            cat.dataset = boat::gdal::create(path.data(), driver.data());
        check(cat);
    }
}

BOOST_AUTO_TEST_CASE(gdal_raster)
{
    auto cat1 = boat::slippy::catalog{};
    cat1.url = "http://basemaps.cartocdn.com/light_all/{z}/{x}/{y}.png";
    cat1.zmax = 2;
    auto rast1 = cat1.get_raster(cat1.layers().at(0));

    auto z = boat::tile::zmax(rast1.width, rast1.height);
    auto tiles = boat::tile::all(rast1.width, rast1.height, z) |
                 std::views::take(16) | std::ranges::to<std::vector>();
    BOOST_CHECK(!tiles.empty());
    auto img1 = cat1.read(rast1, tiles) | std::ranges::to<std::map>();
    BOOST_CHECK_EQUAL(img1.size(), tiles.size());

    struct {
        char const* path;
        char const* driver;
    } tests[] = {
        {"./drop.gdal_raster.tif", "GTiff"},
        {"./drop.gdal_raster.gpkg", "GPKG"},
    };
    for (auto [path, driver] : tests) {
        auto cat2 = boat::gdal::catalog{};
        cat2.dataset = boat::gdal::create(path, driver, rast1);
        auto rast2 = cat2.get_raster(cat2.layers().at(0));

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
}

BOOST_AUTO_TEST_CASE(gdal_image_io)
{
    namespace gil = boost::gil;
    boat::gdal::init();
    auto ds = boat::gdal::dataset_ptr{
        GDALCreate(GDALGetDriverByName("MEM"), "", 4, 4, 3, GDT_Byte, nullptr)};
    BOOST_REQUIRE(ds);
    auto expected = gil::rgb8_image_t{4, 4};
    for (int y = 0; y < 4; ++y)
        for (int x = 0; x < 4; ++x)
            gil::view(expected)(x, y) = gil::rgb8_pixel_t(
                1 + x + 4 * y, 21 + x + 4 * y, 41 + x + 4 * y);
    auto check = [&](auto img, int margin) {
        auto v = gil::view(img);
        auto n = 4 - 2 * margin;
        auto crop = gil::subimage_view(v, margin, margin, n, n);
        auto background = gil::rgb8_pixel_t{201, 202, 203};
        gil::fill_pixels(v, background);
        gil::copy_pixels(gil::subimage_view(gil::const_view(expected),
                                           margin, margin, n, n), crop);
        auto want = img;
        auto actual = gil::rgb8_image_t{4, 4};
        for (int i = 0; i < 3; ++i)
            BOOST_REQUIRE_EQUAL(
                GDALFillRaster(GDALGetRasterBand(ds.get(), i + 1), 201 + i, 0),
                CE_None);
        boat::gdal::image_io(ds.get(), GF_Write, margin, margin, n, n, crop);
        boat::gdal::image_io(ds.get(), GF_Read, 0, 0, 4, 4, gil::view(actual));
        boat::gdal::image_io(
            ds.get(), GF_Write, 0, 0, 4, 4, gil::const_view(expected));
        gil::fill_pixels(v, background);
        boat::gdal::image_io(ds.get(), GF_Read, margin, margin, n, n, crop);
        for (int y = 0; y < 4; ++y)
            for (int x = 0; x < 4; ++x) {
                BOOST_CHECK(gil::const_view(actual)(x, y) ==
                            gil::const_view(want)(x, y));
                BOOST_CHECK(v(x, y) == gil::const_view(want)(x, y));
            }
    };
    for (size_t alignment : {0, 16})
        for (int margin : {0, 1}) {
            check(gil::rgb8_image_t{4, 4, alignment}, margin);
            check(gil::rgb8_planar_image_t{4, 4, alignment}, margin);
        }
}

BOOST_AUTO_TEST_CASE(gdal_raster_edge_tiles)
{
    for (auto [width, height] : {
             std::pair{512, 512},
             {513, 513},
             {515, 515},
             {513, 1},
             {1, 515},
             {1, 1},
         }) {
        auto cat = boat::gdal::catalog{};
        auto rast = boat::db::raster{
            .bands{{"gray", "byte"}},
            .width = width,
            .height = height,
            .xscale = 1,
            .yscale = -1,
            .epsg = 4326,
        };
        cat.dataset = boat::gdal::create("", "MEM", rast);
        BOOST_REQUIRE_EQUAL(
            GDALFillRaster(GDALGetRasterBand(cat.dataset.get(), 1), 29, 0),
            CE_None);
        for (int z = 0; z <= boat::tile::zmax(width, height); ++z) {
            auto tiles = boat::tile::all(width, height, z) |
                         std::ranges::to<std::vector>();
            auto images = cat.read(rast, tiles) | std::ranges::to<std::map>();
            BOOST_REQUIRE_EQUAL(images.size(), tiles.size());
            for (auto& [t, img] : images) {
                auto [x, y, w, h] = t.rect(width, height);
                auto scale = boat::tile::scale(width, height, z);
                BOOST_REQUIRE_EQUAL(img.width(), std::ceil(double(w) / scale));
                BOOST_REQUIRE_EQUAL(img.height(), std::ceil(double(h) / scale));
                auto [a] = t.affine(width, height);
                BOOST_CHECK_EQUAL(a[0][2], x);
                BOOST_CHECK_EQUAL(a[1][2], y);
                BOOST_CHECK_SMALL(a[0][2] + a[0][0] * img.width() - (x + w),
                                  1e-10);
                BOOST_CHECK_SMALL(a[1][2] + a[1][1] * img.height() - (y + h),
                                  1e-10);
                auto v = const_view(get<boost::gil::gray8_image_t>(img));
                BOOST_CHECK_EQUAL(v(0, 0)[0], 29);
                BOOST_CHECK_EQUAL(v(v.width() - 1, v.height() - 1)[0], 29);
            }
        }
    }
}
