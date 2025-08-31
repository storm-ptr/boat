// Andrew Naplavkov

#ifndef BOAT_PFR_ADAPTED_GEOMETRY_HPP
#define BOAT_PFR_ADAPTED_GEOMETRY_HPP

#include <boat/geometry/wkb.hpp>
#include <boat/pfr/variant.hpp>

namespace boat::pfr {

void read(variant const& in, geometry::ogc99 auto& out)
{
    if (in.has_value())
        blob_view{std::get<blob>(in)} >> out;
    else
        out = {};
}

void write(variant& out, geometry::ogc99 auto const& in)
{
    out.emplace<blob>() << in;
}

}  // namespace boat::pfr

#endif  // BOAT_PFR_ADAPTED_GEOMETRY_HPP
