// Andrew Naplavkov

#ifndef BOAT_GIL_HPP
#define BOAT_GIL_HPP

#include <boat/blob.hpp>
#include <boost/gil/extension/dynamic_image/any_image.hpp>
#if __has_include(<png.h>) && __has_include(<zlib.h>)
#include <boost/gil/extension/io/png.hpp>
#endif
#include <boost/mp11.hpp>
#include <cstdint>

namespace boat::gil {

template <class Value, class Layout>
using image = boost::gil::image<boost::gil::pixel<Value, Layout>, false>;

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
            boost::gil::abgr_layout_t,
            boost::gil::argb_layout_t,
            boost::gil::bgr_layout_t,
            boost::gil::bgra_layout_t,
            boost::gil::cmyk_layout_t,
            boost::gil::gray_layout_t,
            boost::gil::rgb_layout_t,
            boost::gil::rgba_layout_t>>,
    boost::gil::any_image>;

blob to_png(specialized<boost::gil::any_image_view> auto img)
{
#if __has_include(<png.h>) && __has_include(<zlib.h>)
    auto os = std::ostringstream{std::ios_base::out | std::ios_base::binary};
    boost::gil::write_view(os, img, boost::gil::png_tag());
    auto view = os.view();
    return {as_bytes(view.data()), view.size()};
#else
    throw std::runtime_error("compiled without libpng/zlib");
#endif
}

inline any_image from_png(blob_view png)
{
#if __has_include(<png.h>) && __has_include(<zlib.h>)
    auto ret = any_image{};
    auto is = std::istringstream(std::string(as_chars(png.data()), png.size()));
    boost::gil::read_image(is, ret, boost::gil::png_tag());
    return ret;
#else
    throw std::runtime_error("compiled without libpng/zlib");
#endif
}

}  // namespace boat::gil

#endif  // BOAT_GIL_HPP
