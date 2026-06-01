// Andrew Naplavkov

#ifndef BOAT_GDAL_GIL_HPP
#define BOAT_GDAL_GIL_HPP

#include <boat/detail/gil.hpp>
#include <boat/gdal/detail/utility.hpp>

namespace boat::gdal {

template <class T>
consteval GDALColorInterp as_color()
{
    // clang-format off
    return std::same_as<T, boost::gil::alpha_t>      ? GCI_AlphaBand
         : std::same_as<T, boost::gil::black_t>      ? GCI_BlackBand
         : std::same_as<T, boost::gil::blue_t>       ? GCI_BlueBand
         : std::same_as<T, boost::gil::cyan_t>       ? GCI_CyanBand
         : std::same_as<T, boost::gil::gray_color_t> ? GCI_GrayIndex
         : std::same_as<T, boost::gil::green_t>      ? GCI_GreenBand
         : std::same_as<T, boost::gil::magenta_t>    ? GCI_MagentaBand
         : std::same_as<T, boost::gil::red_t>        ? GCI_RedBand
         : std::same_as<T, boost::gil::yellow_t>     ? GCI_YellowBand
         : throw std::out_of_range("GDALColorInterp");
    // clang-format on
}

template <class T>
consteval auto as_colors()
{
    constexpr int num_channels = boost::gil::num_channels<T>::value;
    auto ret = std::array<GDALColorInterp, num_channels>{};
    [&]<int... Is>(std::integer_sequence<int, Is...>) {
        ((ret[Is] = as_color<boost::mp11::mp_at_c<
              typename boost::gil::color_space_type<T>::type,
              boost::mp11::mp_at_c<
                  typename boost::gil::channel_mapping_type<T>::type,
                  Is>::value>>()),
         ...);
    }(std::make_integer_sequence<int, num_channels>{});
    return ret;
}

template <class T, range_of<GDALColorInterp> R>
auto color_mapping(R&& colors)
{
    auto ret = std::vector<int>{};
    for (auto c : as_colors<T>()) {
        auto it = std::ranges::find(colors, c);
        boat::check(it != colors.end(), GDALGetColorInterpretationName(c));
        ret.push_back(static_cast<int>(std::distance(colors.begin(), it)));
    }
    return ret;
}

template <class T = uint8_t>
gil::any_image make_image(  //
    std::vector<GDALColorInterp> const& colors,
    int width,
    int height)
{
    auto is = [&](auto... cs) { return colors == std::vector{cs...}; };
    if (is(GCI_AlphaBand, GCI_BlueBand, GCI_GreenBand, GCI_RedBand))
        return gil::image<T, boost::gil::abgr_layout_t>{width, height};
    if (is(GCI_AlphaBand, GCI_RedBand, GCI_GreenBand, GCI_BlueBand))
        return gil::image<T, boost::gil::argb_layout_t>{width, height};
    if (is(GCI_BlueBand, GCI_GreenBand, GCI_RedBand))
        return gil::image<T, boost::gil::bgr_layout_t>{width, height};
    if (is(GCI_BlueBand, GCI_GreenBand, GCI_RedBand, GCI_AlphaBand))
        return gil::image<T, boost::gil::bgra_layout_t>{width, height};
    if (is(GCI_CyanBand, GCI_MagentaBand, GCI_YellowBand, GCI_BlackBand))
        return gil::image<T, boost::gil::cmyk_layout_t>{width, height};
    if (is(GCI_GrayIndex))
        return gil::image<T, boost::gil::gray_layout_t>{width, height};
    if (is(GCI_RedBand, GCI_GreenBand, GCI_BlueBand))
        return gil::image<T, boost::gil::rgb_layout_t>{width, height};
    if (is(GCI_RedBand, GCI_GreenBand, GCI_BlueBand, GCI_AlphaBand))
        return gil::image<T, boost::gil::rgba_layout_t>{width, height};
    auto os = std::ostringstream{} << "gdal::make_image";
    for (auto sep = " "; auto c : colors)
        os << std::exchange(sep, ", ") << GDALGetColorInterpretationName(c);
    throw std::runtime_error{os.str()};
}

}  // namespace boat::gdal

#endif  // BOAT_GDAL_GIL_HPP
