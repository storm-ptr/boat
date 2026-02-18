// Andrew Naplavkov

#ifndef BOAT_GDAL_FIELDS_HPP
#define BOAT_GDAL_FIELDS_HPP

#include <boat/gdal/detail/fields/attribute.hpp>
#include <boat/gdal/detail/fields/geometry.hpp>

namespace boat::gdal::fields {

using field = std::variant<attribute, geometry>;

inline field make(OGRFeatureDefnH fd, char const* name)
{
    if (auto i = OGR_FD_GetFieldIndex(fd, name); i >= 0)
        return attribute::make(fd, i);
    if (auto i = OGR_FD_GetGeomFieldIndex(fd, name); i >= 0)
        return geometry::make(fd, i);
    throw std::runtime_error(name);
}

std::vector<field> make(OGRFeatureDefnH fd, range_of<std::string> auto&& names)
{
    auto fn = [&](auto& name) { return make(fd, name.data()); };
    return names | std::views::transform(fn) | std::ranges::to<std::vector>();
}

inline std::vector<field> make(OGRFeatureDefnH fd)
{
    auto ret = std::vector<field>{};
    for (int i = 0, n = OGR_FD_GetFieldCount(fd); i < n; ++i)
        ret.push_back(attribute::make(fd, i));
    for (int i = 0, n = OGR_FD_GetGeomFieldCount(fd); i < n; ++i)
        ret.push_back(geometry::make(fd, i));
    return ret;
}

}  // namespace boat::gdal::fields

#endif  // BOAT_GDAL_FIELDS_HPP
