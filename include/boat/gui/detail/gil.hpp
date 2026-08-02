// Andrew Naplavkov

#ifndef BOAT_GUI_GIL_HPP
#define BOAT_GUI_GIL_HPP

#include <boat/geometry/vocabulary.hpp>
#include <boost/gil.hpp>

namespace boat::gui {

auto get_pixel(specialized<boost::gil::image_view> auto image)
{
    return [=](geometry::point auto const& pixel) {
        auto x = pixel.x(), y = pixel.y();
        return x >= 0 && x < image.width() && y >= 0 && y < image.height()
                   ? std::optional{image(x, y)}
                   : std::nullopt;
    };
}

}  // namespace boat::gui

#endif  // BOAT_GUI_GIL_HPP
