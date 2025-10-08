// Andrew Naplavkov

#ifndef BOAT_GDAL_META_HPP
#define BOAT_GDAL_META_HPP

#include <boat/gdal/detail/utility.hpp>

namespace boat::gdal {

struct band {
    GDALColorInterp color;
    GDALDataType type;
};

struct meta {
    std::string file;
    int width;
    int height;
    std::vector<band> bands;
    geometry::matrix affine;
    int epsg;
};

auto& operator<<(ostream auto& out, meta const& in)
{
    out << "{ file: " << in.file << "\n";
    out << ", width: " << in.width << "\n";
    out << ", height: " << in.height << "\n";
    out << ", bands: {";
    for (auto sep = ""; auto& band : in.bands)
        out << std::exchange(sep, ", ")
            << GDALGetColorInterpretationName(band.color) << ":"
            << GDALGetDataTypeName(band.type);
    out << "}\n";
    out << ", affine: [";
    for (auto sep1 = ""; auto& row : in.affine.a) {
        out << std::exchange(sep1, ", ") << "[";
        for (auto sep2 = ""; auto& val : row)
            out << std::exchange(sep2, ", ") << val;
        out << "]";
    }
    out << "]\n";
    return out << ", epsg: " << in.epsg << " }\n";
}

}  // namespace boat::gdal

#endif  // BOAT_GDAL_META_HPP
