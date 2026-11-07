// Andrew Naplavkov

#include <QStringList>
#include <algorithm>
#include <boat/gdal/drivers.hpp>
#include <ranges>
#include "formats.h"

namespace {

QString make_filter(std::string const& long_name, std::string const& extensions)
{
    auto pattern = QString::fromStdString(extensions);
    pattern.replace(' ', " *.");
    return QString::fromStdString(long_name) + " (" +
           (extensions.empty() ? "" : "*.") + pattern + ')';
}

}  // namespace

std::vector<copy_format> copy_formats(bool raster)
{
    auto ret = std::vector<copy_format>{};
    auto type = raster ? boat::gdal::driver_type::Raster
                       : boat::gdal::driver_type::Vector;
    for (auto drv : boat::gdal::drivers(type, boat::gdal::driver_op::Create))
        ret.emplace_back(
            make_filter(drv.long_name, drv.extensions),
            QString::fromStdString(drv.short_name),
            QString::fromStdString(
                "." + drv.extensions.substr(0, drv.extensions.find(' '))));
    std::ranges::stable_sort(ret, {}, &copy_format::filter);
    return ret;
}

QString copy_as_filter(std::span<copy_format const> fmts)
{
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
