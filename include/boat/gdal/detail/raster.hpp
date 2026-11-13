// Andrew Naplavkov

#ifndef BOAT_GDAL_RASTER_HPP
#define BOAT_GDAL_RASTER_HPP

#include <array>
#include <boat/db/meta.hpp>
#include <boat/gdal/detail/gil.hpp>
#include <boat/gdal/detail/image_io.hpp>

namespace boat::gdal {

inline auto get_bands(GDALDatasetH ds)
{
    return std::views::iota(0, GDALGetRasterCount(ds)) |
           std::views::transform([=](int i) -> db::band {
               auto b = GDALGetRasterBand(ds, i + 1);
               return {
                   to_lower(GDALGetColorInterpretationName(
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
        .extent{GDALGetRasterXSize(ds), GDALGetRasterYSize(ds)},
        .xorig = a[0],
        .yorig = a[3],
        .xscale = a[1],
        .yscale = a[5],
        .xskew = a[2],
        .yskew = a[4],
        .srid = epsg,
        .epsg = epsg,
        .wkt = get_wkt(crs),
        .proj4 = get_proj4(crs)};
}

auto to_colors(range_of<db::band> auto&& bands)
{
    return bands | std::views::transform([](auto& b) {
               return GDALGetColorInterpretationByName(b.color_name.data());
           }) |
           std::ranges::to<std::vector>();
}

inline gil::any_image
read(GDALDatasetH ds, db::raster const& rast, tile const& t)
{
    auto cs = to_colors(rast.bands);
    auto ret = [&] {
        switch (GDALGetDataTypeByName(rast.bands.at(0).type_name.data())) {
            case GDT_Byte:
                return make_image<uint8_t>(cs);
            case GDT_Int8:
                return make_image<int8_t>(cs);
            case GDT_UInt16:
                return make_image<uint16_t>(cs);
            case GDT_Int16:
                return make_image<int16_t>(cs);
            case GDT_UInt32:
                return make_image<uint32_t>(cs);
            case GDT_Int32:
                return make_image<int32_t>(cs);
            case GDT_UInt64:
                return make_image<uint64_t>(cs);
            case GDT_Int64:
                return make_image<int64_t>(cs);
            case GDT_Float32:
                return make_image<float>(cs);
            case GDT_Float64:
                return make_image<double>(cs);
            default:
                throw std::runtime_error(rast.bands.at(0).type_name);
        }
    }();
    auto rect = t.rect(rast.extent);
    auto [x, y, w, h] = geometry::xywh(rect);
    auto px = tile::scale(rast.extent, t.z);
    ret.recreate(w / px + (w % px != 0), h / px + (h % px != 0));
    visit([&](auto v) { image_io(ds, GF_Read, rect, v); }, view(ret));
    return ret;
}

inline void write(
    GDALDatasetH ds,
    db::raster const& rast,
    geometry::raster::box const& rect,
    gil::any_image_view img)
{
    visit(
        [&]<class T>(T v) {
            image_io(
                ds, GF_Write, rect, v, color_mapping<T>(to_colors(rast.bands)));
        },
        img);
}

}  // namespace boat::gdal

#endif  // BOAT_GDAL_RASTER_HPP
