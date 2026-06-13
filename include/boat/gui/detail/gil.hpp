// Andrew Naplavkov

#ifndef BOAT_GUI_GIL_HPP
#define BOAT_GUI_GIL_HPP

#include <boat/geometry/vocabulary.hpp>
#include <boost/gil.hpp>

namespace boat::gui {

auto get_pixel(specialized<boost::gil::image_view> auto img)
{
    return [=](geometry::point auto const& p) {
        return between<int>(p.x(), 0, img.width() - 1) &&
                       between<int>(p.y(), 0, img.height() - 1)
                   ? std::optional{img(p.x(), p.y())}
                   : std::nullopt;
    };
}

}  // namespace boat::gui

#endif  // BOAT_GUI_GIL_HPP
