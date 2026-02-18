// Andrew Naplavkov

#ifndef BOAT_GDAL_RASTER_HPP
#define BOAT_GDAL_RASTER_HPP

#include <boat/gdal/dataset.hpp>
#include <boat/gdal/detail/adapted/transform.hpp>

namespace boat::gdal {

struct band {
    std::string lcase_color;
    std::string lcase_type;
};

struct raster {
    int width;
    int height;
    std::vector<band> bands;
    geometry::matrix affine;
    int epsg;

    auto to_colors() const
    {
        auto fn = [](auto& v) { return v.lcase_color.data(); };
        return bands | std::views::transform(fn);
    }

    friend auto& operator<<(ostream auto& out, raster const& in)
    {
        out << "{ width: " << in.width << "\n";
        out << ", height: " << in.height << "\n";
        out << ", bands: {";
        for (auto sep = ""; auto& band : in.bands)
            out << std::exchange(sep, ", ") << band.lcase_color << ":"
                << band.lcase_type;
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
};

inline dataset_ptr create(char const* file, char const* driver, raster const& r)
{
    init();
    auto drv = GDALGetDriverByName(driver);
    boat::check(!!drv, driver);
    auto ret = dataset_ptr{GDALCreate(  //
        drv,
        file,
        r.width,
        r.height,
        static_cast<int>(r.bands.size()),
        GDALGetDataTypeByName(r.bands.at(0).lcase_type.data()),
        0)};
    boat::check(!!ret, "GDALCreate");
    set_transform(ret.get(), r.affine);
    check(GDALSetSpatialRef(ret.get(), make_epsg_srs(r.epsg).get()));
    return ret;
}

inline std::optional<raster> describe(GDALDatasetH ds)
{
    int num_bands = GDALGetRasterCount(ds);
    if (num_bands <= 0)
        return std::nullopt;
    auto to_band = [=](int i) -> band {
        auto b = GDALGetRasterBand(ds, i + 1);
        return {to_lower(GDALGetColorInterpretationName(
                    GDALGetRasterColorInterpretation(b))),
                to_lower(GDALGetDataTypeName(GDALGetRasterDataType(b)))};
    };
    return {raster{
        .width = GDALGetRasterXSize(ds),
        .height = GDALGetRasterYSize(ds),
        .bands{std::from_range,
               std::views::iota(0, num_bands) | std::views::transform(to_band)},
        .affine{get_transform(ds)},
        .epsg = get_authority_code(GDALGetSpatialRef(ds)),
    }};
}

}  // namespace boat::gdal

#endif  // BOAT_GDAL_RASTER_HPP
