// Andrew Naplavkov

#ifndef BOAT_GUI_FEATURE_HPP
#define BOAT_GUI_FEATURE_HPP

#include <boat/blob.hpp>
#include <boost/qvm/mat.hpp>
#include <variant>

namespace boat::gui {

struct raster {
    blob image;
    boost::qvm::mat<double, 3, 3> affine;
    int epsg;
};

struct shape {
    blob wkb;
    int epsg;
};

using feature = std::variant<raster, shape>;

}  // namespace boat::gui

#endif  // BOAT_GUI_FEATURE_HPP
