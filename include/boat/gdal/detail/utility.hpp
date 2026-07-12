// Andrew Naplavkov

#ifndef BOAT_GDAL_UTILITY_HPP
#define BOAT_GDAL_UTILITY_HPP

#include <cpl_conv.h>
#include <cpl_string.h>
#include <gdal.h>
#include <ogr_srs_api.h>
#include <boat/detail/string.hpp>
#include <cstring>
#include <generator>
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
using srs_ptr = unique_ptr<void, OSRRelease>;
using string_ptr = unique_ptr<char, CPLFree>;

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
        throw std::runtime_error(error_or(concat("gdal CPLErr ", ec)));
}

inline void check(OGRErr ec)
{
    if (OGRERR_NONE != ec)
        throw std::runtime_error(error_or(concat("gdal OGRErr ", ec)));
}

inline srs_ptr make_srs(  //
    int epsg,
    std::string const& wkt,
    std::string const& proj4)
{
    auto ret = srs_ptr{OSRNewSpatialReference(0)};
    boat::check(!!ret, "OSRNewSpatialReference");
    if (epsg > 0)
        check(OSRImportFromEPSG(ret.get(), epsg));
    else if (auto ptr = const_cast<char*>(wkt.data()); !wkt.empty())
        check(OSRImportFromWkt(ret.get(), &ptr));
    else if (!proj4.empty())
        check(OSRImportFromProj4(ret.get(), proj4.data()));
    else
        throw std::runtime_error("no spatial reference");
    return ret;
}

inline int get_epsg(OGRSpatialReferenceH crs)
{
    auto name = OSRGetAuthorityName(crs, 0);
    auto code = OSRGetAuthorityCode(crs, 0);
    if (!name || !code || std::strcmp(name, "EPSG"))
        return 0;
    return from_chars<int>(code, std::strlen(code));
}

inline std::string get_proj4(OGRSpatialReferenceH crs)
{
    char* ptr = nullptr;
    check(OSRExportToProj4(crs, &ptr));
    auto str = string_ptr{ptr};
    if (!ptr)
        return {};
    return {ptr};
}

inline std::string get_wkt(OGRSpatialReferenceH crs)
{
    char* ptr = nullptr;
    check(OSRExportToWktEx(crs, &ptr, nullptr));
    auto str = string_ptr{ptr};
    if (!ptr)
        return {};
    return {ptr};
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

inline layer_ptr execute(GDALDatasetH ds, char const* sql, char const* dialect)
{
    return {GDALDatasetExecuteSQL(ds, sql, 0, dialect), layer_deleter{ds}};
}

inline void set_autocommit(GDALDatasetH ds, bool on)
{
    if (!GDALDatasetTestCapability(ds, ODsCTransactions))
        return;
    check(on ? GDALDatasetRollbackTransaction(ds)
             : GDALDatasetStartTransaction(ds, false));
}

inline void commit(GDALDatasetH ds)
{
    if (!GDALDatasetTestCapability(ds, ODsCTransactions))
        return;
    check(GDALDatasetCommitTransaction(ds));
    check(GDALDatasetStartTransaction(ds, false));
}

}  // namespace boat::gdal

#endif  // BOAT_GDAL_UTILITY_HPP
