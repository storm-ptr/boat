// Andrew Naplavkov

#ifndef BOAT_GUI_FEATURE_HPP
#define BOAT_GUI_FEATURE_HPP

#include <boat/blob.hpp>
#include <boat/geometry/model.hpp>

namespace boat::gui {

struct feature {
    blob raster;
    geometry::geographic::variant shape;  ///< geographic or cartesian
    int epsg;
};

}  // namespace boat::gui

#endif  // BOAT_GUI_FEATURE_HPP
