// Andrew Naplavkov

#ifndef BOAT_GDAL_FIELDS_ATTRIBUTE_HPP
#define BOAT_GDAL_FIELDS_ATTRIBUTE_HPP

#include <boat/gdal/detail/adapted/date_time.hpp>
#include <boat/pfr/variant.hpp>

namespace boat::gdal::fields {

struct attribute {
    std::string name;
    OGRFieldType type;
    int index;

    static attribute make(OGRFeatureDefnH fd, int index)
    {
        auto fld = OGR_FD_GetFieldDefn(fd, index);
        return {.name = OGR_Fld_GetNameRef(fld),
                .type = OGR_Fld_GetType(fld),
                .index = index};
    }

    pfr::variant read(OGRFeatureH feat) const
    {
        if (OGR_F_IsFieldNull(feat, index))
            return {};
        switch (type) {
            case OFTDateTime:
                return pfr::to_variant(get_date_time(feat, index));
            case OFTInteger:
            case OFTInteger64:
                return OGR_F_GetFieldAsInteger64(feat, index);
            case OFTReal:
                return OGR_F_GetFieldAsDouble(feat, index);
            case OFTString:
                return OGR_F_GetFieldAsString(feat, index);
            case OFTBinary: {
                int len;
                auto ptr = OGR_F_GetFieldAsBinary(feat, index, &len);
                return pfr::variant{
                    std::in_place_type<blob>,
                    as_bytes(ptr),
                    static_cast<size_t>(len),
                };
            }
            default:
                throw std::runtime_error(
                    concat(name, " ", OGR_GetFieldTypeName(type)));
        }
    }

    void write(OGRFeatureH feat, pfr::variant const& var) const
    {
        if (!var)
            return OGR_F_SetFieldNull(feat, index);
        switch (type) {
            case OFTDateTime:
                return set_date_time(feat, index, pfr::get<time_point>(var));
            case OFTInteger:
            case OFTInteger64:
                return OGR_F_SetFieldInteger64(
                    feat, index, pfr::get<int64_t>(var));
            case OFTReal:
                return OGR_F_SetFieldDouble(feat, index, pfr::get<double>(var));
            case OFTString:
                return OGR_F_SetFieldString(
                    feat, index, std::get<std::string>(var).data());
            case OFTBinary: {
                auto const& buf = std::get<blob>(var);
                return OGR_F_SetFieldBinary(
                    feat, index, static_cast<int>(buf.size()), buf.data());
            }
            default:
                throw std::runtime_error(
                    concat(name, " ", OGR_GetFieldTypeName(type)));
        }
    }
};

}  // namespace boat::gdal::fields

#endif  // BOAT_GDAL_FIELDS_ATTRIBUTE_HPP
