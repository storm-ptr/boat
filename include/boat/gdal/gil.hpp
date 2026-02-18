// Andrew Naplavkov

#ifndef BOAT_GDAL_GIL_HPP
#define BOAT_GDAL_GIL_HPP

#include <boat/gdal/detail/utility.hpp>
#include <boost/gil/extension/dynamic_image/any_image.hpp>

namespace boat::gdal {

using any_image = boost::gil::any_image<  //
    boost::gil::abgr8_image_t,
    boost::gil::argb8_image_t,
    boost::gil::bgr8_image_t,
    boost::gil::bgra8_image_t,
    boost::gil::cmyk8_image_t,
    boost::gil::gray8_image_t,
    boost::gil::rgb8_image_t,
    boost::gil::rgba8_image_t>;

any_image make_image(int width, int height, range_of<char const*> auto&& colors)
{
    auto vals = colors |
                std::views::transform(&GDALGetColorInterpretationByName) |
                std::ranges::to<std::vector>();
    auto eq = [&](std::initializer_list<GDALColorInterp> expect) {
        return std::ranges::equal(vals, expect);
    };
    if (eq({GCI_AlphaBand, GCI_BlueBand, GCI_GreenBand, GCI_RedBand}))
        return boost::gil::abgr8_image_t{width, height};
    if (eq({GCI_AlphaBand, GCI_RedBand, GCI_GreenBand, GCI_BlueBand}))
        return boost::gil::argb8_image_t{width, height};
    if (eq({GCI_BlueBand, GCI_GreenBand, GCI_RedBand}))
        return boost::gil::bgr8_image_t{width, height};
    if (eq({GCI_BlueBand, GCI_GreenBand, GCI_RedBand, GCI_AlphaBand}))
        return boost::gil::bgra8_image_t{width, height};
    if (eq({GCI_CyanBand, GCI_MagentaBand, GCI_YellowBand, GCI_BlackBand}))
        return boost::gil::cmyk8_image_t{width, height};
    if (eq({GCI_GrayIndex}))
        return boost::gil::gray8_image_t{width, height};
    if (eq({GCI_RedBand, GCI_GreenBand, GCI_BlueBand}))
        return boost::gil::rgb8_image_t{width, height};
    if (eq({GCI_RedBand, GCI_GreenBand, GCI_BlueBand, GCI_AlphaBand}))
        return boost::gil::rgba8_image_t{width, height};
    auto os = std::ostringstream{};
    os << "make_image";
    for (auto sep = " "; auto val : vals)
        os << std::exchange(sep, ", ") << GDALGetColorInterpretationName(val);
    throw std::runtime_error{os.str()};
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
    T const& img,
    std::vector<int> bands = {})
{
    namespace gil = boost::gil;
    using channel_t = gil::channel_type<T>::type;
    using value_t = gil::channel_traits<channel_t>::value_type;
    constexpr auto planar = gil::is_planar<T>::value;
    if (bands.empty())
        bands.assign_range(std::views::iota(0u, gil::num_channels<T>::value));
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
        ds,
        rw,
        x,
        y,
        width,
        height,
        data,
        static_cast<int>(img.width()),
        static_cast<int>(img.height()),
        std::visit(to_data_type, std::variant<value_t>{}),
        static_cast<int>(bands.size()),
        bands.data(),
        static_cast<int>(pixel),
        static_cast<int>(line),
        static_cast<int>(band)));
}

}  // namespace boat::gdal

#endif  // BOAT_GDAL_GIL_HPP
