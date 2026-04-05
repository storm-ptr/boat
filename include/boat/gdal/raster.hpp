// Andrew Naplavkov

#ifndef BOAT_GDAL_RASTER_HPP
#define BOAT_GDAL_RASTER_HPP

#include <boat/gdal/dataset.hpp>
#include <boat/gdal/gil.hpp>
#if __has_include(<png.h>) && __has_include(<zlib.h>)
#include <boost/gil/extension/io/png.hpp>
#endif

namespace boat::gdal {

inline db::band get_band(GDALDatasetH ds, int i)
{
    auto b = GDALGetRasterBand(ds, i + 1);
    return {to_lower(GDALGetColorInterpretationName(
                GDALGetRasterColorInterpretation(b))),
            to_lower(GDALGetDataTypeName(GDALGetRasterDataType(b)))};
}

inline db::raster get_raster(GDALDatasetH ds)
{
    auto a = std::array<double, 6>{};
    check(GDALGetGeoTransform(ds, a.data()));
    return {
        .table_name{"_layer"},
        .column_name{"_raster"},
        .bands{
            std::from_range,
            std::views::iota(0, GDALGetRasterCount(ds)) |
                std::views::transform([=](int i) { return get_band(ds, i); })},
        .width = GDALGetRasterXSize(ds),
        .height = GDALGetRasterYSize(ds),
        .xorig = a[0],
        .yorig = a[3],
        .xscale = a[1],
        .yscale = a[5],
        .xskew = a[2],
        .yskew = a[4],
        .epsg = get_authority_code(GDALGetSpatialRef(ds)),
    };
}

#if __has_include(<png.h>) && __has_include(<zlib.h>)
inline blob get_png(GDALDatasetH ds, db::raster const& r, tile const& t)
{
    auto scale = tile::scale(r.width, r.height, t.z);
    auto [x1, y1] = t.min_corner(r.width, r.height);
    auto [x2, y2] = t.max_corner(r.width, r.height);
    auto w = x2 - x1;
    auto h = y2 - y1;
    auto layout =
        r.bands | std::views::transform([](auto& b) {
            return GDALGetColorInterpretationByName(b.color_name.data());
        }) |
        std::ranges::to<std::vector>();
    auto img = make_image(layout, w / scale, h / scale);
    boost::variant2::visit(
        [&](auto& v) { image_io(ds, GF_Read, x1, y1, w, h, gil::view(v)); },
        img);
    auto os = std::ostringstream{std::ios_base::out | std::ios_base::binary};
    gil::write_view(os, gil::view(img), gil::png_tag());
    auto str = std::move(os).str();
    return {as_bytes(str.data()), str.size()};
}
#endif

}  // namespace boat::gdal

#endif  // BOAT_GDAL_RASTER_HPP
