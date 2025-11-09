// Andrew Naplavkov

#ifndef BOAT_GUI_FEATURE_HPP
#define BOAT_GUI_FEATURE_HPP

#include <boat/blob.hpp>
#include <boat/geometry/vocabulary.hpp>
#include <variant>

namespace boat::gui {

struct raster {
    blob image;
    geometry::matrix affine;
    int epsg;
};

struct shape {
    blob wkb;
    int epsg;
};

using feature = std::variant<raster, shape>;

}  // namespace boat::gui

#endif  // BOAT_GUI_FEATURE_HPP
