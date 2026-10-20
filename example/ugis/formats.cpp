// Andrew Naplavkov

#include <QStringList>
#include <boat/detail/string.hpp>
#include <boat/gdal/drivers.hpp>
#include "formats.h"

namespace {

QString make_filter(std::string const& long_name, std::string const& extensions)
{
    auto pattern = QString::fromStdString(extensions);
    pattern.replace(' ', " *.");
    return QString::fromStdString(long_name) + " (" +
           (extensions.empty() ? "" : "*.") + pattern + ')';
}

boat::gdal::driver_type format_type(bool raster)
{
    return raster ? boat::gdal::driver_type::Raster
                  : boat::gdal::driver_type::Vector;
}

std::vector<copy_format> make_formats(  //
    boat::gdal::driver_type type,
    boat::gdal::driver_op op)
{
    auto ret = std::vector<copy_format>{};
    for (auto drv : boat::gdal::drivers(type, op))
        ret.emplace_back(
            make_filter(drv.long_name, drv.extensions),
            QString::fromStdString(drv.short_name),
            QString::fromStdString(
                "." + drv.extensions.substr(0, drv.extensions.find(' '))));
    return ret;
}

}  // namespace

std::optional<copy_format> copy_as_format(bool raster, QString const& filter)
{
    for (auto& fmt :
         make_formats(format_type(raster), boat::gdal::driver_op::Create))
        if (filter == fmt.filter)
            return fmt;
    return std::nullopt;
}

QString copy_as_filter(bool raster)
{
    auto fmts =
        make_formats(format_type(raster), boat::gdal::driver_op::Create);
    std::ranges::sort(fmts);
    return (fmts | std::views::transform(&copy_format::filter) |
            std::ranges::to<QStringList>())
        .join(";;");
}

QString open_filter()
{
    auto filters = QStringList{};
    for (auto type :
         {boat::gdal::driver_type::Raster, boat::gdal::driver_type::Vector})
        for (auto drv : boat::gdal::drivers(type, boat::gdal::driver_op::Open))
            filters.push_back(make_filter(drv.long_name, drv.extensions));
    filters.removeDuplicates();
    filters.sort();
    filters.push_back("All files (*.*)");
    return filters.join(";;");
}

QString ensure_extension(QString path, QString const& extension)
{
    if (!path.endsWith(extension, Qt::CaseInsensitive))
        path += extension;
    return path;
}
