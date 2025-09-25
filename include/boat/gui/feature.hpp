// Andrew Naplavkov

#ifndef BOAT_GUI_FEATURE_HPP
#define BOAT_GUI_FEATURE_HPP

#include <boat/blob.hpp>

namespace boat::gui {

struct feature {
    blob raster;
    blob shape;
    int epsg;
};

}  // namespace boat::gui

#endif  // BOAT_GUI_FEATURE_HPP
