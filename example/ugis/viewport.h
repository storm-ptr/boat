// Andrew Naplavkov

#ifndef VIEWPORT_H
#define VIEWPORT_H

#include <boat/geometry/vocabulary.hpp>
#include <boat/gui/scene.hpp>

struct viewport {
    boat::gui::view view;
    boat::geometry::raster::point extent;
};

#endif  // VIEWPORT_H
