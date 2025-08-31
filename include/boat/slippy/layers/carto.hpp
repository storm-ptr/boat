// Andrew Naplavkov

#ifndef BOAT_SLIPPY_LAYERS_CARTO_HPP
#define BOAT_SLIPPY_LAYERS_CARTO_HPP

#include <boat/detail/utility.hpp>
#include <boat/slippy/layers/layer.hpp>

namespace boat::slippy::layers::carto {

struct light_all : layer {
    std::string_view company_name() const override { return "carto"; }
    std::string_view layer_name() const override { return "light_all"; }

    std::string url(tile const& t) const override
    {
        return concat("http://basemaps.cartocdn.com/light_all/",
                      t.z,
                      '/',
                      t.x,
                      '/',
                      t.y,
                      ".png");
    }
};

}  // namespace boat::slippy::layers::carto

#endif  // BOAT_SLIPPY_LAYERS_CARTO_HPP
