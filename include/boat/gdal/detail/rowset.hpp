// Andrew Naplavkov

#ifndef BOAT_GDAL_ROWSET_HPP
#define BOAT_GDAL_ROWSET_HPP

#include <boat/db/rowset.hpp>
#include <boat/gdal/detail/fields/fields.hpp>

namespace boat::gdal {

db::rowset read(OGRLayerH lyr, range_of<fields::field> auto&& flds, int limit)
{
    auto ret = db::rowset{};
    for (auto& fld : flds)
        ret.columns.push_back(std::visit([&](auto& v) { return v.name; }, fld));
    for (int i = 0; i < limit; ++i) {
        auto feat = feature_ptr{OGR_L_GetNextFeature(lyr)};
        if (!feat)
            break;
        for (auto& row = ret.rows.emplace_back(); auto& fld : flds)
            row.push_back(
                std::visit([&](auto& v) { return v.read(feat.get()); }, fld));
    }
    return ret;
}

inline void write(OGRLayerH lyr, db::rowset const& vals)
{
    auto fd = OGR_L_GetLayerDefn(lyr);
    auto flds = fields::make(fd, vals.columns);
    for (auto const& row : vals) {
        auto feat = feature_ptr{OGR_F_Create(fd)};
        for (auto&& [fld, var] : std::views::zip(flds, row))
            std::visit([&](auto& v) { v.write(feat.get(), var); }, fld);
        check(OGR_L_CreateFeature(lyr, feat.get()));
    }
}

}  // namespace boat::gdal

#endif  // BOAT_GDAL_ROWSET_HPP
