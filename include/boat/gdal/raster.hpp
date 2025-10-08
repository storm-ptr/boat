// Andrew Naplavkov

#ifndef BOAT_GDAL_RASTER_HPP
#define BOAT_GDAL_RASTER_HPP

#include <boat/gdal/meta.hpp>
#include <boost/gil.hpp>

namespace boat::gdal {

class raster {
    std::string file_;
    unique_ptr<void, GDALClose> ds_;

    band get_band(int i) const
    {
        auto b = GDALGetRasterBand(ds_.get(), i + 1);
        return {GDALGetRasterColorInterpretation(b), GDALGetRasterDataType(b)};
    }

public:
    explicit raster(char const* file) : file_(file)
    {
        init();
        ds_.reset(GDALOpenEx(file, 0, 0, 0, 0));
        boat::check(!!ds_, file);
    }

    raster(char const* driver, meta const& m) : file_(m.file)
    {
        init();
        auto drv = GDALGetDriverByName(driver);
        boat::check(!!drv, driver);
        char** opts = 0;
        ds_.reset(GDALCreate(  //
            drv,
            m.file.data(),
            m.width,
            m.height,
            static_cast<int>(m.bands.size()),
            m.bands.at(0).type,
            opts));
        boat::check(!!ds_, "GDALCreate");
        check(GDALSetGeoTransform(ds_.get(), encode(m.affine).data()));
        auto srs = unique_ptr<void, OSRDestroySpatialReference>{
            OSRNewSpatialReference(0)};
        boat::check(!!srs, "OSRNewSpatialReference");
        check(OSRImportFromEPSG(srs.get(), m.epsg));
        check(GDALSetSpatialRef(ds_.get(), srs.get()));
    }

    meta get_meta() const
    {
        auto gt = geo_transform{};
        check(GDALGetGeoTransform(ds_.get(), gt.data()));
        return {
            .file{file_},
            .width = GDALGetRasterXSize(ds_.get()),
            .height = GDALGetRasterYSize(ds_.get()),
            .bands{
                std::from_range,
                std::views::iota(0, GDALGetRasterCount(ds_.get())) |
                    std::views::transform([&](int i) { return get_band(i); })},
            .affine{decode(gt)},
            .epsg = authority_code(GDALGetSpatialRef(ds_.get())),
        };
    }

    template <GDALRWFlag RW = GF_Read, specialized<boost::gil::image_view> T>
        requires(!specialized<typename T::value_type, boost::gil::packed_pixel>)
    void io(  //
        int x,
        int y,
        int width,
        int height,
        T img,
        std::vector<int> bands = {}) const
    {
        namespace gil = boost::gil;
        using channel_t = gil::channel_type<T>::type;
        using value_t = gil::channel_traits<channel_t>::value_type;
        constexpr auto num_channels = gil::num_channels<T>::value;
        constexpr auto planar = gil::is_planar<T>::value;
        if (bands.empty())
            bands.assign_range(std::views::iota(0u, num_channels));
        for (int& i : bands)
            ++i;
        void* data;
        if constexpr (planar)
            data = (void*)gil::planar_view_get_raw_data(img, 0);
        else
            data = (void*)gil::interleaved_view_get_raw_data(img);
        auto pixel = sizeof(value_t) * (planar ? 1 : bands.size());
        auto line = pixel * img.width();
        auto band = planar ? line * img.height() : sizeof(value_t);
        check(GDALDatasetRasterIO(  //
            ds_.get(),
            RW,
            x,
            y,
            width,
            height,
            data,
            static_cast<int>(img.width()),
            static_cast<int>(img.height()),
            encode<value_t>(),
            static_cast<int>(bands.size()),
            bands.data(),
            static_cast<int>(pixel),
            static_cast<int>(line),
            static_cast<int>(band)));
    }
};

}  // namespace boat::gdal

#endif  // BOAT_GDAL_RASTER_HPP
