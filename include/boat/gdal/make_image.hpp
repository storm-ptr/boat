// Andrew Naplavkov

#ifndef BOAT_GDAL_MAKE_IMAGE_HPP
#define BOAT_GDAL_MAKE_IMAGE_HPP

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

any_image make_image(  //
    int width,
    int height,
    range_of<GDALColorInterp> auto&& colors)
{
    auto in = std::vector<GDALColorInterp>{std::from_range, colors};
    auto eq = [&](std::initializer_list<GDALColorInterp> expect) {
        return std::ranges::equal(in, expect);
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
    os << "make_image: ";
    for (auto sep = ""; auto color : in)
        os << std::exchange(sep, ", ") << GDALGetColorInterpretationName(color);
    throw std::runtime_error{os.str()};
}

}  // namespace boat::gdal

#endif  // BOAT_GDAL_MAKE_IMAGE_HPP
