// Andrew Naplavkov

#ifndef BOAT_GDAL_UTILITY_HPP
#define BOAT_GDAL_UTILITY_HPP

#include <cpl_conv.h>
#include <cpl_string.h>
#include <gdal.h>
#include <ogr_srs_api.h>
#include <boat/detail/config.hpp>
#include <boat/detail/string.hpp>
#include <cstring>
#include <generator>
#include <mutex>

namespace boat::gdal {

using dataset_ptr = unique_ptr<void, GDALClose>;
using feature_ptr = unique_ptr<void, OGR_F_Destroy>;
using string_ptr = unique_ptr<char, CPLFree>;

inline void init()
{
    static auto flag = std::once_flag{};
    constexpr auto fn = [] {
        auto sec = std::to_string(std::chrono::seconds{timeout}.count());
        CPLSetConfigOption("GDAL_FILENAME_IS_UTF8", "YES");
        CPLSetConfigOption("GDAL_HTTP_CONNECTTIMEOUT", sec.data());
        CPLSetConfigOption("GDAL_HTTP_TIMEOUT", sec.data());
        GDALAllRegister();
    };
    std::call_once(flag, fn);
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

inline auto make_srs(int epsg, std::string const& wkt, std::string const& proj4)
{
    auto ret = unique_ptr<void, OSRRelease>{OSRNewSpatialReference(0)};
    boat::check(!!ret, "OSRNewSpatialReference");
    if (epsg > 0)
        check(OSRImportFromEPSG(ret.get(), epsg));
    else if (auto ptr = const_cast<char*>(wkt.data()); !wkt.empty())
        check(OSRImportFromWkt(ret.get(), &ptr));
    else if (!proj4.empty())
        check(OSRImportFromProj4(ret.get(), proj4.data()));
    else
        throw std::runtime_error("no SRS");
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
    char* ptr;
    check(OSRExportToProj4(crs, &ptr));
    auto str = string_ptr{ptr};
    if (!ptr)
        return {};
    return {ptr};
}

inline std::string get_wkt(OGRSpatialReferenceH crs)
{
    char* ptr;
    check(OSRExportToWktEx(crs, &ptr, 0));
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

inline auto execute(GDALDatasetH ds, char const* sql, char const* dialect)
{
    struct del {
        GDALDatasetH ds;

        void operator()(OGRLayerH lyr) const
        {
            GDALDatasetReleaseResultSet(ds, lyr);
        }
    };
    return std::unique_ptr<void, del>{
        GDALDatasetExecuteSQL(ds, sql, 0, dialect), del{ds}};
}

inline void set_autocommit(GDALDatasetH ds, bool on)
{
    if (!GDALDatasetTestCapability(ds, ODsCTransactions))
        return;
    check(on ? GDALDatasetRollbackTransaction(ds)
             : GDALDatasetStartTransaction(ds, 0));
}

inline void commit(GDALDatasetH ds)
{
    if (!GDALDatasetTestCapability(ds, ODsCTransactions))
        return;
    check(GDALDatasetCommitTransaction(ds));
    check(GDALDatasetStartTransaction(ds, 0));
}

}  // namespace boat::gdal

#endif  // BOAT_GDAL_UTILITY_HPP
