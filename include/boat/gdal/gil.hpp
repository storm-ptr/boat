// Andrew Naplavkov

#ifndef BOAT_GDAL_GIL_HPP
#define BOAT_GDAL_GIL_HPP

#include <boat/gdal/detail/utility.hpp>
#include <boost/gil/extension/dynamic_image/any_image.hpp>
#include <boost/mp11.hpp>

namespace boat::gdal {

namespace gil = boost::gil;

template <class Value, class Layout>
using image = gil::image<gil::pixel<Value, Layout>, false>;

using any_image = boost::mp11::mp_rename<  //
    boost::mp11::mp_product<               //
        image,
        boost::mp11::mp_list<  //
            uint8_t,
            int8_t,
            uint16_t,
            int16_t,
            uint32_t,
            int32_t,
            float,
            double>,
        boost::mp11::mp_list<  //
            gil::abgr_layout_t,
            gil::argb_layout_t,
            gil::bgr_layout_t,
            gil::bgra_layout_t,
            gil::cmyk_layout_t,
            gil::gray_layout_t,
            gil::rgb_layout_t,
            gil::rgba_layout_t>>,
    gil::any_image>;

template <class Value = uint8_t>
any_image make_image(  //
    std::span<GDALColorInterp const> layout,
    int width,
    int height)
{
    auto eq = [&](std::initializer_list<GDALColorInterp> expect) {
        return std::ranges::equal(layout, expect);
    };
    if (eq({GCI_AlphaBand, GCI_BlueBand, GCI_GreenBand, GCI_RedBand}))
        return image<Value, gil::abgr_layout_t>{width, height};
    if (eq({GCI_AlphaBand, GCI_RedBand, GCI_GreenBand, GCI_BlueBand}))
        return image<Value, gil::argb_layout_t>{width, height};
    if (eq({GCI_BlueBand, GCI_GreenBand, GCI_RedBand}))
        return image<Value, gil::bgr_layout_t>{width, height};
    if (eq({GCI_BlueBand, GCI_GreenBand, GCI_RedBand, GCI_AlphaBand}))
        return image<Value, gil::bgra_layout_t>{width, height};
    if (eq({GCI_CyanBand, GCI_MagentaBand, GCI_YellowBand, GCI_BlackBand}))
        return image<Value, gil::cmyk_layout_t>{width, height};
    if (eq({GCI_GrayIndex}))
        return image<Value, gil::gray_layout_t>{width, height};
    if (eq({GCI_RedBand, GCI_GreenBand, GCI_BlueBand}))
        return image<Value, gil::rgb_layout_t>{width, height};
    if (eq({GCI_RedBand, GCI_GreenBand, GCI_BlueBand, GCI_AlphaBand}))
        return image<Value, gil::rgba_layout_t>{width, height};
    auto os = std::ostringstream{};
    os << "make_image";
    for (auto sep = " "; auto color : layout)
        os << std::exchange(sep, ", ") << GDALGetColorInterpretationName(color);
    throw std::runtime_error{os.str()};
}

template <specialized<gil::image_view> T>
    requires(!specialized<typename T::value_type, gil::packed_pixel>)
void image_io(  //
    GDALDatasetH ds,
    GDALRWFlag rw,
    int x,
    int y,
    int width,
    int height,
    T const& image_view,
    std::vector<int> bands = {})
{
    using channel_t = gil::channel_type<T>::type;
    using value_t = gil::channel_traits<channel_t>::value_type;
    constexpr auto planar = gil::is_planar<T>::value;
    if (bands.empty())
        bands.assign_range(std::views::iota(0u, gil::num_channels<T>::value));
    for (int& i : bands)
        ++i;
    void* data;
    if constexpr (planar)
        data = (void*)gil::planar_view_get_raw_data(image_view, 0);
    else
        data = (void*)gil::interleaved_view_get_raw_data(image_view);
    auto pixel = sizeof(value_t) * (planar ? 1 : bands.size());
    auto line = pixel * image_view.width();
    auto band = planar ? line * image_view.height() : sizeof(value_t);
    check(GDALDatasetRasterIO(  //
        ds,
        rw,
        x,
        y,
        width,
        height,
        data,
        static_cast<int>(image_view.width()),
        static_cast<int>(image_view.height()),
        as_data_type<value_t>(),
        static_cast<int>(bands.size()),
        bands.data(),
        static_cast<int>(pixel),
        static_cast<int>(line),
        static_cast<int>(band)));
}

}  // namespace boat::gdal

#endif  // BOAT_GDAL_GIL_HPP
