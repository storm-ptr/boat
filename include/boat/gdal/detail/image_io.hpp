// Andrew Naplavkov

#ifndef BOAT_GDAL_IMAGE_IO_HPP
#define BOAT_GDAL_IMAGE_IO_HPP

#include <boat/gdal/detail/utility.hpp>
#include <boost/gil.hpp>
#include <cstdint>

namespace boat::gdal {

template <class T>
consteval GDALDataType as_data_type()
{
    // clang-format off
    return std::same_as<T, uint8_t>  ? GDT_Byte
         : std::same_as<T, int8_t>   ? GDT_Int8
         : std::same_as<T, uint16_t> ? GDT_UInt16
         : std::same_as<T, int16_t>  ? GDT_Int16
         : std::same_as<T, uint32_t> ? GDT_UInt32
         : std::same_as<T, int32_t>  ? GDT_Int32
         : std::same_as<T, uint64_t> ? GDT_UInt64
         : std::same_as<T, int64_t>  ? GDT_Int64
         : std::same_as<T, float>    ? GDT_Float32
         : std::same_as<T, double>   ? GDT_Float64
         : throw std::out_of_range("GDALDataType");
    // clang-format on
}

template <specialized<boost::gil::image_view> T>
    requires(!specialized<typename T::value_type, boost::gil::packed_pixel>)
void image_io(  //
    GDALDatasetH ds,
    GDALRWFlag rw,
    int x,
    int y,
    int width,
    int height,
    T img,
    std::vector<int> bands = {})
{
    using value_t = boost::gil::channel_traits<
        typename boost::gil::channel_type<T>::type>::value_type;
    constexpr int num_channels = boost::gil::num_channels<T>::value;
    constexpr bool planar = boost::gil::is_planar<T>::value;
    if (bands.empty())
        bands.assign_range(std::views::iota(0, num_channels));
    for (int& i : bands)
        ++i;
    void* data;
    if constexpr (planar)
        data = (void*)boost::gil::planar_view_get_raw_data(img, 0);
    else
        data = (void*)boost::gil::interleaved_view_get_raw_data(img);
    auto pixel = sizeof(value_t) * (planar ? 1 : num_channels);
    auto line = pixel * img.width();
    auto band = planar ? line * img.height() : sizeof(value_t);
    check(GDALDatasetRasterIO(  //
        ds,
        rw,
        x,
        y,
        width,
        height,
        data,
        static_cast<int>(img.width()),
        static_cast<int>(img.height()),
        as_data_type<value_t>(),
        static_cast<int>(bands.size()),
        bands.data(),
        static_cast<int>(pixel),
        static_cast<int>(line),
        static_cast<int>(band)));
}

}  // namespace boat::gdal

#endif  // BOAT_GDAL_IMAGE_IO_HPP
