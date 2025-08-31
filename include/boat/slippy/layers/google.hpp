// Andrew Naplavkov

#ifndef BOAT_SLIPPY_LAYERS_GOOGLE_HPP
#define BOAT_SLIPPY_LAYERS_GOOGLE_HPP

#include <boat/detail/utility.hpp>
#include <boat/slippy/layers/layer.hpp>

namespace boat::slippy::layers::google {

struct satellite : layer {
    std::string_view company_name() const override { return "google"; }
    std::string_view layer_name() const override { return "satellite"; }

    std::string url(tile const& t) const override
    {
        return concat(
            "http://mt.google.com/vt/lyrs=s&x=", t.x, "&y=", t.y, "&z=", t.z);
    }
};

}  // namespace boat::slippy::layers::google

#endif  // BOAT_SLIPPY_LAYERS_GOOGLE_HPP
