// Andrew Naplavkov

#ifndef BOAT_GUI_SCENE_JSON_HPP
#define BOAT_GUI_SCENE_JSON_HPP

#include <boat/gui/detail/json.hpp>

namespace boat::gui {

/// Parse without interpreting addresses or generating names.
inline scene read_scene_json(std::string const& text)
{
    auto root = json::parse(text);
    check(root.is_object(), "scene");
    auto ret = scene{};
    for (auto const& source : array(root, "sources"))
        ret.sources.push_back(read_source(source));
    if (root.contains("center")) {
        auto const& coords = array(root, "center");
        check(coords.size() == 2, "center");
        ret.view.center = {coords[0].get<double>(), coords[1].get<double>()};
    }
    ret.view.resolution = root.value("resolution", ret.view.resolution);
    check(ret.view.resolution > 0, "resolution");
    return ret;
}

inline std::string write_scene_json(scene const& in)
{
    check(std::isfinite(in.view.center.x()) &&
              std::isfinite(in.view.center.y()) &&
              std::isfinite(in.view.resolution) && in.view.resolution > 0,
          "view");
    auto root = json::object();
    for (auto const& source : in.sources)
        root["sources"].push_back(write_source(source));
    if (in.view.center.x() != 0 || in.view.center.y() != 0)
        root["center"] = {in.view.center.x(), in.view.center.y()};
    if (in.view.resolution != view{}.resolution)
        root["resolution"] = in.view.resolution;
    return root.dump(2);
}

}  // namespace boat::gui

#endif  // BOAT_GUI_SCENE_JSON_HPP
