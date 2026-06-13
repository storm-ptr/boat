// Andrew Naplavkov

#ifndef BOAT_GIL_HPP
#define BOAT_GIL_HPP

#include <boat/blob.hpp>
#include <boost/gil/extension/dynamic_image/any_image.hpp>
#if __has_include(<jpeglib.h>)
#include <boost/gil/extension/io/jpeg.hpp>
#else
#pragma message("no libjpeg")
#endif
#if __has_include(<png.h>) && __has_include(<zlib.h>)
#include <boost/gil/extension/io/png.hpp>
#else
#pragma message("no libpng/zlib")
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
            uint64_t,
            int64_t,
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

using any_image_view =
    std::decay_t<decltype(const_view(std::declval<any_image>()))>;

template <class T>
    requires requires { boost::gil::view(std::declval<T&>()); }
T to(any_image_view img)
{
    auto ret = T{img.dimensions()};
    copy_and_convert_pixels(img, view(ret));
    return ret;
}

template <class T>
    requires requires { boost::gil::view(std::declval<T&>()); }
T read(blob_view img)
{
    auto fit = [](std::initializer_list<int> bytes) {
        return bytes | std::views::transform([](int i) {
                   return static_cast<std::byte>(i);
               }) |
               std::ranges::to<blob>();
    };
    auto is = std::istringstream(std::string(as_chars(img.data()), img.size()));
    auto ret = T{};
    if (img.starts_with(fit({0x89, 0x50, 0x4E, 0x47, 0x0D, 0x0A, 0x1A, 0x0A})))
#if __has_include(<png.h>) && __has_include(<zlib.h>)
        read_and_convert_image(is, ret, boost::gil::png_tag());
#else
        throw std::runtime_error("compiled without libpng/zlib");
#endif
    else if (img.starts_with(fit({0xFF, 0xD8, 0xFF})))
#if __has_include(<jpeglib.h>)
        read_and_convert_image(is, ret, boost::gil::jpeg_tag());
#else
        throw std::runtime_error("compiled without libjpeg");
#endif
    else
        throw std::runtime_error("unsupported image format");
    return ret;
}

}  // namespace boat::gil

#endif  // BOAT_GIL_HPP
