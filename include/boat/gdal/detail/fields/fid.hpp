// Andrew Naplavkov

#ifndef BOAT_GDAL_FIELDS_FID_HPP
#define BOAT_GDAL_FIELDS_FID_HPP

#include <boat/db/adapted/adapted.hpp>
#include <boat/gdal/detail/adapted/date_time.hpp>

namespace boat::gdal::fields {

struct fid {
    static constexpr auto kind = db::kind<int>::value;
    static constexpr auto type = OFTInteger64;
    std::string name;

    static std::optional<fid> make(OGRLayerH lyr)
    {
        if (auto col = OGR_L_GetFIDColumn(lyr); col && std::strlen(col))
            return fid{col};
        return std::nullopt;
    }

    db::variant read(OGRFeatureH feat) const { return OGR_F_GetFID(feat); }

    void write(OGRFeatureH feat, db::variant const& var) const
    {
        check(OGR_F_SetFID(feat, var ? db::get<int64_t>(var) : OGRNullFID));
    }
};

}  // namespace boat::gdal::fields

#endif  // BOAT_GDAL_FIELDS_FID_HPP
