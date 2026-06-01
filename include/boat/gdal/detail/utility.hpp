// Andrew Naplavkov

#ifndef BOAT_GDAL_UTILITY_HPP
#define BOAT_GDAL_UTILITY_HPP

#include <cpl_conv.h>
#include <cpl_string.h>
#include <gdal.h>
#include <ogr_srs_api.h>
#include <boat/detail/string.hpp>
#include <cstring>
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

inline srs_ptr make_epsg_srs(int epsg)
{
    auto ret = srs_ptr{OSRNewSpatialReference(0)};
    boat::check(!!ret, "OSRNewSpatialReference");
    check(OSRImportFromEPSG(ret.get(), epsg));
    return ret;
}

inline int get_epsg_code(OGRSpatialReferenceH crs)
{
    if (!crs)
        return 0;
    auto name = OSRGetAuthorityName(crs, 0);
    auto code = OSRGetAuthorityCode(crs, 0);
    if (!name || !code || std::strcmp(name, "EPSG"))
        return 0;
    return from_chars<int>(code, std::strlen(code));
}

inline layer_ptr execute(GDALDatasetH ds, char const* sql, char const* dialect)
{
    return {GDALDatasetExecuteSQL(ds, sql, 0, dialect), layer_deleter{ds}};
}

inline auto subdatasets(GDALDatasetH ds)
    -> std::generator<std::pair<char const*, char const*>>
{
    auto meta = GDALGetMetadata(ds, "SUBDATASETS");
    if (!CSLCount(meta))
        co_return;
    for (int i = 1;; ++i) {
        auto desc =
            CSLFetchNameValue(meta, concat("SUBDATASET_", i, "_DESC").data());
        auto name =
            CSLFetchNameValue(meta, concat("SUBDATASET_", i, "_NAME").data());
        if (!desc || !name)
            co_return;
        co_yield {desc, name};
    }
}

}  // namespace boat::gdal

#endif  // BOAT_GDAL_UTILITY_HPP
