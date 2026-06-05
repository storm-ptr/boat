// Andrew Naplavkov

#ifndef BOAT_GDAL_RASTER_HPP
#define BOAT_GDAL_RASTER_HPP

#include <array>
#include <boat/db/meta.hpp>
#include <boat/gdal/detail/gil.hpp>
#include <boat/gdal/image_io.hpp>

namespace boat::gdal {

inline auto get_bands(GDALDatasetH ds)
{
    return std::views::iota(0, GDALGetRasterCount(ds)) |
           std::views::transform([=](int i) -> db::band {
               auto b = GDALGetRasterBand(ds, i + 1);
               return {to_lower(GDALGetColorInterpretationName(
                           GDALGetRasterColorInterpretation(b))),
                       to_lower(GDALGetDataTypeName(GDALGetRasterDataType(b)))};
           }) |
           std::ranges::to<std::vector>();
}

inline db::raster get_raster(GDALDatasetH ds)
{
    auto a = std::array<double, 6>{};
    check(GDALGetGeoTransform(ds, a.data()));
    auto crs = GDALGetSpatialRef(ds);
    auto epsg = get_epsg(crs);
    return {
        .table_name{"_layer"},
        .column_name{"raster"},
        .bands{get_bands(ds)},
        .width = GDALGetRasterXSize(ds),
        .height = GDALGetRasterYSize(ds),
        .xorig = a[0],
        .yorig = a[3],
        .xscale = a[1],
        .yscale = a[5],
        .xskew = a[2],
        .yskew = a[4],
        .srid = epsg,
        .epsg = epsg,
        .wkt = get_wkt(crs),
        .proj4 = get_proj4(crs),
    };
}

auto to_colors(range_of<db::band> auto&& bands)
{
    return bands | std::views::transform([](auto& b) {
               return GDALGetColorInterpretationByName(b.color_name.data());
           }) |
           std::ranges::to<std::vector>();
}

inline blob read(GDALDatasetH ds, db::raster const& rast, tile const& t)
{
    auto scale = tile::scale(rast.width, rast.height, t.z);
    auto [x, y, w, h] = t.rect(rast.width, rast.height);
    auto img = make_image(to_colors(rast.bands), w / scale, h / scale);
    boost::variant2::visit(
        [&](auto& v) {
            image_io(ds, GF_Read, x, y, w, h, boost::gil::view(v));
        },
        img);
    return gil::write_png(boost::gil::view(img));
}

inline void write(  //
    GDALDatasetH ds,
    db::raster const& rast,
    db::rect const& rect,
    blob_view data)
{
    boost::variant2::visit(
        [&]<class T>(T const& v) {
            image_io(  //
                ds,
                GF_Write,
                rect.x,
                rect.y,
                rect.width,
                rect.height,
                boost::gil::const_view(v),
                color_mapping<T>(to_colors(rast.bands)));
        },
        gil::read_image(data));
}

}  // namespace boat::gdal

#endif  // BOAT_GDAL_RASTER_HPP
