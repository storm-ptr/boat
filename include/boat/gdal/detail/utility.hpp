// Andrew Naplavkov

#ifndef BOAT_GDAL_UTILITY_HPP
#define BOAT_GDAL_UTILITY_HPP

#include <cpl_conv.h>
#include <gdal.h>
#include <ogr_srs_api.h>
#include <array>
#include <boat/detail/charconv.hpp>
#include <boat/geometry/vocabulary.hpp>
#include <cstdint>
#include <mutex>

namespace boat::gdal {

using geo_transform = std::array<double, 6>;

inline void init()
{
    static auto flag = std::once_flag{};
    std::call_once(flag, [] {
        CPLSetConfigOption("GDAL_HTTP_TIMEOUT", "30");
        CPLSetConfigOption("GDAL_FILENAME_IS_UTF8", "YES");
        GDALAllRegister();
    });
}

inline std::string error()
{
    auto ret = std::string{CPLGetLastErrorMsg()};
    if (ret.empty())
        ret = "gdal";
    return ret;
}

inline void check(CPLErr ec)
{
    if (CE_None != ec)
        throw std::runtime_error(error());
}

inline void check(OGRErr ec)
{
    if (OGRERR_NONE != ec)
        throw std::runtime_error(error());
}

inline int authority_code(OGRSpatialReferenceH srs)
{
    auto code = std::string_view{srs ? OSRGetAuthorityCode(srs, 0) : "0"};
    return from_chars<int>(code.data(), code.size());
}

template <arithmetic T>
constexpr GDALDataType encode()
{
    constexpr auto vis = overloaded{
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
    return std::visit(vis, std::variant<T>{});
}

constexpr geometry::matrix decode(geo_transform const& gt)
{
    return {{{gt[1], gt[2], gt[0]}, {gt[4], gt[5], gt[3]}, {0., 0., 1.}}};
}

constexpr geo_transform encode(geometry::matrix const& mat)
{
    auto& a = mat.a;
    return {{a[0][2], a[0][0], a[0][1], a[1][2], a[1][0], a[1][1]}};
}

}  // namespace boat::gdal

#endif  // BOAT_GDAL_UTILITY_HPP
