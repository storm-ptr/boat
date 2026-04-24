// Andrew Naplavkov

#ifndef BOAT_GDAL_UTILITY_HPP
#define BOAT_GDAL_UTILITY_HPP

#include <cpl_conv.h>
#include <cpl_string.h>
#include <gdal.h>
#include <ogr_srs_api.h>
#include <array>
#include <boat/detail/string.hpp>
#include <mutex>

namespace boat::gdal {

struct layer_deleter {
    GDALDatasetH dataset;

    void operator()(OGRLayerH lyr) const
    {
        GDALDatasetReleaseResultSet(dataset, lyr);
    }
};

using dataset_ptr = unique_ptr<void, GDALClose>;
using feature_ptr = unique_ptr<void, OGR_F_Destroy>;
using layer_ptr = std::unique_ptr<void, layer_deleter>;
using srs_ptr = unique_ptr<void, OSRDestroySpatialReference>;

template <class T>
consteval GDALDataType as_data_type()
{
    return  //
        std::same_as<T, uint8_t>    ? GDT_Byte
        : std::same_as<T, int8_t>   ? GDT_Int8
        : std::same_as<T, uint16_t> ? GDT_UInt16
        : std::same_as<T, int16_t>  ? GDT_Int16
        : std::same_as<T, uint32_t> ? GDT_UInt32
        : std::same_as<T, int32_t>  ? GDT_Int32
        : std::same_as<T, uint64_t> ? GDT_UInt64
        : std::same_as<T, int64_t>  ? GDT_Int64
        : std::same_as<T, float>    ? GDT_Float32
        : std::same_as<T, double>   ? GDT_Float64
                                    : throw std::out_of_range("GDALDataType");
}

inline void init()
{
    static auto flag = std::once_flag{};
    std::call_once(flag, [] {
        CPLSetConfigOption("GDAL_FILENAME_IS_UTF8", "YES");
        CPLSetConfigOption("GDAL_HTTP_TIMEOUT", "30");
        CPLSetConfigOption("SHAPE_ENCODING", "UTF-8");
        GDALAllRegister();
    });
}

inline std::string error_or(std::string_view default_value)
{
    auto ret = std::string{CPLGetLastErrorMsg()};
    if (ret.empty())
        ret = default_value;
    return ret;
}

inline void check(CPLErr ec)
{
    if (CE_None != ec)
        throw std::runtime_error(error_or("gdal"));
}

inline void check(OGRErr ec)
{
    if (OGRERR_NONE != ec)
        throw std::runtime_error(error_or("gdal"));
}

srs_ptr make_epsg_srs(int epsg)
{
    auto ret = srs_ptr{OSRNewSpatialReference(0)};
    boat::check(!!ret, "OSRNewSpatialReference");
    check(OSRImportFromEPSG(ret.get(), epsg));
    return ret;
}

inline int get_authority_code(OGRSpatialReferenceH crs)
{
    auto code = std::string_view{crs ? OSRGetAuthorityCode(crs, 0) : "0"};
    return from_chars<int>(code.data(), code.size());
}

inline layer_ptr execute(GDALDatasetH ds, char const* sql, char const* dialect)
{
    return {GDALDatasetExecuteSQL(ds, sql, 0, dialect), layer_deleter{ds}};
}

}  // namespace boat::gdal

#endif  // BOAT_GDAL_UTILITY_HPP
