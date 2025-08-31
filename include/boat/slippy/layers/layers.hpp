// Andrew Naplavkov

#ifndef BOAT_SLIPPY_LAYERS_HPP
#define BOAT_SLIPPY_LAYERS_HPP

#include <boat/slippy/layers/carto.hpp>
#include <boat/slippy/layers/google.hpp>
#include <boat/slippy/layers/osm.hpp>
#include <boost/fusion/algorithm.hpp>
#include <boost/fusion/container.hpp>

namespace boat::slippy::layers {

constexpr auto all =
    boost::fusion::list<carto::light_all, google::satellite, osm::standard>{};

inline layer const& find(std::string_view company_name,
                         std::string_view layer_name)
{
    auto ptr = static_cast<layer const*>(nullptr);
    boost::fusion::for_each(all, [&](auto& lyr) {
        if (!ptr && std::pair{lyr.company_name(), lyr.layer_name()} ==
                        std::pair{company_name, layer_name})
            ptr = &lyr;
    });
    if (ptr)
        return *ptr;
    throw std::runtime_error(concat(company_name, '.', layer_name));
}

}  // namespace boat::slippy::layers

#endif  // BOAT_SLIPPY_LAYERS_HPP
