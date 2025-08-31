// Andrew Naplavkov

#ifndef BOAT_SLIPPY_TILE_HPP
#define BOAT_SLIPPY_TILE_HPP

#include <boost/pfr/ops_fields.hpp>
#include <functional>

namespace boat::slippy {

constexpr auto zmax = 19;

struct tile {
    int x;
    int y;
    int z;

    friend bool operator==(tile const&, tile const&) = default;
};

}  // namespace boat::slippy

template <>
struct std::hash<boat::slippy::tile> {
    static size_t operator()(boat::slippy::tile const& that)
    {
        return boost::pfr::hash_fields(that);
    }
};

#endif  // BOAT_SLIPPY_TILE_HPP
