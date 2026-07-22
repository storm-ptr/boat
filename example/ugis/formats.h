// Andrew Naplavkov

#ifndef FORMATS_H
#define FORMATS_H

#include <QString>
#include <set>

constexpr auto workspace_filter = "ugis workspace (*.ugis)";

struct copy_format {
    char const* filter;
    char const* driver;
    char const* ext;
};

constexpr copy_format vector_formats[] = {
    {"ESRI Shapefile (*.shp)", "ESRI Shapefile", ".shp"},
    {"GeoJSON (*.geojson *.json)", "GeoJSON", ".geojson"},
    {"GeoPackage (*.gpkg)", "GPKG", ".gpkg"},
    {"SQLite/Spatialite (*.sqlite)", "SQLite", ".sqlite"},
};

constexpr copy_format raster_formats[] = {
    {"Cloud Optimized GeoTIFF (*.tif *.tiff)", "COG", ".tif"},
    {"GeoPackage (*.gpkg)", "GPKG", ".gpkg"},
    {"GeoTIFF (*.tif *.tiff)", "GTiff", ".tif"},
    {"NetCDF (*.nc)", "NetCDF", ".nc"},
};

inline copy_format const* copy_as_format(bool raster, QString const& filter)
{
    auto ptr = raster ? raster_formats : vector_formats;
    auto count = raster ? std::size(raster_formats) : std::size(vector_formats);
    for (auto& fmt : std::span{ptr, count})
        if (filter == fmt.filter)
            return &fmt;
    return nullptr;
}

inline QString copy_as_filter(bool raster)
{
    auto ret = QString{};
    auto ptr = raster ? raster_formats : vector_formats;
    auto count = raster ? std::size(raster_formats) : std::size(vector_formats);
    for (auto sep{""}; auto& fmt : std::span{ptr, count}) {
        ret += std::exchange(sep, ";;");
        ret += fmt.filter;
    }
    ret += ";;All files (*.*)";
    return ret;
}

inline QString open_filter()
{
    auto ret = QString{};
    auto filters = std::set<QString>{};
    for (auto& fmt : raster_formats)
        filters.insert(fmt.filter);
    for (auto& fmt : vector_formats)
        filters.insert(fmt.filter);
    for (auto sep{""}; auto& filter : filters) {
        ret += std::exchange(sep, ";;");
        ret += filter;
    }
    ret += ";;All files (*.*)";
    return ret;
}

inline QString ensure_ext(QString path, char const* ext)
{
    auto suffix = QString::fromLatin1(ext);
    if (!path.endsWith(suffix, Qt::CaseInsensitive))
        path += suffix;
    return path;
}

#endif  // FORMATS_H
