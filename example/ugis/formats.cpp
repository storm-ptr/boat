// Andrew Naplavkov

#include <QTextStream>
#include <boat/detail/string.hpp>
#include <boat/gdal/drivers.hpp>
#include <set>
#include "formats.h"

namespace {

std::string make_filter(std::string_view long_name, std::string_view extensions)
{
    auto os = std::ostringstream{};
    os << long_name << " (";
    for (auto sep{"*."}; auto ext : std::views::split(extensions, ' '))
        os << std::exchange(sep, " *.")
           << std::string_view{ext.data(), ext.size()};
    os << ")";
    return std::move(os).str();
}

std::string make_extension(std::string_view extensions)
{
    for (auto ext : std::views::split(extensions, ' '))
        return boat::concat(".", std::string_view{ext.data(), ext.size()});
    return {};
}

std::vector<copy_format> make_formats(  //
    boat::gdal::driver_type type,
    boat::gdal::driver_op op)
{
    auto ret = std::vector<copy_format>{};
    for (auto drv : boat::gdal::drivers(type, op))
        ret.emplace_back(
            QString::fromStdString(make_filter(drv.long_name, drv.extensions)),
            QString::fromStdString(drv.short_name),
            QString::fromStdString(make_extension(drv.extensions)));
    return ret;
}

}  // namespace

std::optional<copy_format> copy_as_format(bool raster, QString const& filter)
{
    for (auto& fmt : make_formats(  //
             raster ? boat::gdal::driver_type::Raster
                    : boat::gdal::driver_type::Vector,
             boat::gdal::driver_op::Create))
        if (filter == fmt.filter)
            return fmt;
    return std::nullopt;
}

QString copy_as_filter(bool raster)
{
    auto fmts = make_formats(  //
        raster ? boat::gdal::driver_type::Raster
               : boat::gdal::driver_type::Vector,
        boat::gdal::driver_op::Create);
    std::ranges::sort(fmts);
    auto ret = QString{};
    auto os = QTextStream{&ret};
    for (auto sep{""}; auto& fmt : fmts)
        os << std::exchange(sep, ";;") << fmt.filter;
    return ret;
}

QString open_filter()
{
    auto filters = std::set<QString>{};
    for (auto& fmt : make_formats(  //
             boat::gdal::driver_type::Raster,
             boat::gdal::driver_op::Open))
        filters.insert(fmt.filter);
    for (auto& fmt : make_formats(  //
             boat::gdal::driver_type::Vector,
             boat::gdal::driver_op::Open))
        filters.insert(fmt.filter);
    auto ret = QString{};
    auto os = QTextStream{&ret};
    auto sep{""};
    for (auto const& filter : filters)
        os << std::exchange(sep, ";;") << filter;
    os << std::exchange(sep, ";;") << "All files (*.*)";
    return ret;
}

QString ensure_extension(QString path, QString const& extension)
{
    if (!path.endsWith(extension, Qt::CaseInsensitive))
        path += extension;
    return path;
}
