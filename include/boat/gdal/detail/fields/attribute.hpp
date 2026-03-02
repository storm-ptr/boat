// Andrew Naplavkov

#ifndef BOAT_GDAL_FIELDS_ATTRIBUTE_HPP
#define BOAT_GDAL_FIELDS_ATTRIBUTE_HPP

#include <boat/db/adapted/adapted.hpp>
#include <boat/gdal/detail/adapted/date_time.hpp>

namespace boat::gdal::fields {

inline OGRFieldType to_type(std::string_view kind)
{
    if (kind == db::kind<int>::value)
        return OFTInteger64;
    if (kind == db::kind<float>::value)
        return OFTReal;
    if (kind == db::kind<std::string>::value)
        return OFTString;
    if (kind == db::kind<blob>::value)
        return OFTBinary;
    if (kind == db::kind<time_point>::value)
        return OFTDateTime;
    throw concat("OGRFieldType ", kind);
}

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

    std::string_view kind() const
    {
        switch (type) {
            case OFTDateTime:
                return db::kind<time_point>::value;
            case OFTInteger:
            case OFTInteger64:
                return db::kind<int>::value;
            case OFTReal:
                return db::kind<float>::value;
            case OFTString:
                return db::kind<std::string>::value;
            case OFTBinary:
                return db::kind<blob>::value;
            default:
                return {};
        }
    }

    db::variant read(OGRFeatureH feat) const
    {
        if (OGR_F_IsFieldNull(feat, index))
            return {};
        switch (type) {
            case OFTDateTime:
                return db::to_variant(get_date_time(feat, index));
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
                return db::variant{
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

    void write(OGRFeatureH feat, db::variant const& var) const
    {
        if (!var)
            return OGR_F_SetFieldNull(feat, index);
        switch (type) {
            case OFTDateTime:
                return set_date_time(feat, index, db::get<time_point>(var));
            case OFTInteger:
            case OFTInteger64:
                return OGR_F_SetFieldInteger64(
                    feat, index, db::get<int64_t>(var));
            case OFTReal:
                return OGR_F_SetFieldDouble(feat, index, db::get<double>(var));
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
