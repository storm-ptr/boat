// Andrew Naplavkov

#ifndef BOAT_GDAL_UTILITY_HPP
#define BOAT_GDAL_UTILITY_HPP

#include <cpl_conv.h>
#include <gdal.h>
#include <ogr_srs_api.h>
#include <array>
#include <boat/detail/charconv.hpp>
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

constexpr auto to_data_type = overloaded{
    [](uint8_t) { return GDT_Byte; },
    [](int8_t) { return GDT_Int8; },
    [](uint16_t) { return GDT_UInt16; },
    [](int16_t) { return GDT_Int16; },
    [](uint32_t) { return GDT_UInt32; },
    [](int32_t) { return GDT_Int32; },
    [](uint64_t) { return GDT_UInt64; },
    [](int64_t) { return GDT_Int64; },
    [](float) { return GDT_Float32; },
    [](double) { return GDT_Float64; },
};

inline void init()
{
    static auto flag = std::once_flag{};
    std::call_once(flag, [] {
        CPLSetConfigOption("GDAL_HTTP_TIMEOUT", "30");
        CPLSetConfigOption("GDAL_FILENAME_IS_UTF8", "YES");
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

inline int get_authority_code(OGRSpatialReferenceH srs)
{
    auto code = std::string_view{srs ? OSRGetAuthorityCode(srs, 0) : "0"};
    return from_chars<int>(code.data(), code.size());
}

inline layer_ptr execute(GDALDatasetH ds, char const* sql, char const* dialect)
{
    return {GDALDatasetExecuteSQL(ds, sql, 0, dialect), layer_deleter{ds}};
}

}  // namespace boat::gdal

#endif  // BOAT_GDAL_UTILITY_HPP
