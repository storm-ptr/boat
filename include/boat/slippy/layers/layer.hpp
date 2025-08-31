// Andrew Naplavkov

#ifndef BOAT_SLIPPY_LAYERS_LAYER_HPP
#define BOAT_SLIPPY_LAYERS_LAYER_HPP

#include <boat/slippy/tile.hpp>
#include <string>
#include <string_view>

namespace boat::slippy::layers {

struct layer {
    virtual ~layer() = default;
    virtual std::string_view company_name() const = 0;
    virtual std::string_view layer_name() const = 0;
    virtual std::string url(tile const&) const = 0;
};

}  // namespace boat::slippy::layers

#endif  // BOAT_SLIPPY_LAYERS_LAYER_HPP
