// Andrew Naplavkov

#ifndef BOAT_DB_ADAPTED_GEOMETRY_HPP
#define BOAT_DB_ADAPTED_GEOMETRY_HPP

#include <boat/db/variant.hpp>
#include <boat/geometry/wkb.hpp>

namespace boat::db {

/// Reads WKB from a blob; NULL produces a default-constructed geometry.
void read(variant const& in, geometry::ogc99 auto& out)
{
    if (in.has_value())
        blob_view{std::get<blob>(in)} >> out;
    else
        out = {};
}

/// Serializes an OGC geometry as WKB in a blob.
void write(variant& out, geometry::ogc99 auto const& in)
{
    out.emplace<blob>() << in;
}

template <geometry::ogc99 T>
struct kind<T> {
    static constexpr auto value = "geometry";
};

}  // namespace boat::db

#endif  // BOAT_DB_ADAPTED_GEOMETRY_HPP
