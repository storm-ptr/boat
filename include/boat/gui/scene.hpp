// Andrew Naplavkov

#ifndef BOAT_GUI_SCENE_HPP
#define BOAT_GUI_SCENE_HPP

#include <boat/db/meta.hpp>
#include <boat/geometry/vocabulary.hpp>
#include <optional>

namespace boat::gui {

struct view {
    geometry::geographic::point center{0, 0};
    double resolution = 9'783.94;  ///< metres per pixel
};

struct layer {
    db::layer db{};
    uint32_t color = 0xff000000;  ///< ARGB
    double width = 1;             ///< pixels
    std::optional<uint32_t> fill;
    bool hidden = false;
};

struct source {
    db::source db;
    std::vector<std::variant<layer, source>> children;
};

struct scene {
    std::vector<source> sources;
    gui::view view;
};

}  // namespace boat::gui

#endif  // BOAT_GUI_SCENE_HPP
