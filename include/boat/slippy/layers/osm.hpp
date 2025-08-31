// Andrew Naplavkov

#ifndef BOAT_SLIPPY_LAYERS_OSM_HPP
#define BOAT_SLIPPY_LAYERS_OSM_HPP

#include <boat/detail/utility.hpp>
#include <boat/slippy/layers/layer.hpp>

namespace boat::slippy::layers::osm {

struct standard : layer {
    std::string_view company_name() const override { return "osm"; }
    std::string_view layer_name() const override { return "standard"; }

    std::string url(tile const& t) const override
    {
        return concat(
            "http://tile.openstreetmap.org/", t.z, '/', t.x, '/', t.y, ".png");
    }
};

}  // namespace boat::slippy::layers::osm

#endif  // BOAT_SLIPPY_LAYERS_OSM_HPP
