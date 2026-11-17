// Andrew Naplavkov

#ifndef BOAT_TILE_HPP
#define BOAT_TILE_HPP

#include <boat/geometry/vocabulary.hpp>
#include <boost/pfr/ops_fields.hpp>
#include <generator>

namespace boat {

struct tile {
    int z, y, x;

    static constexpr int size = 256;  ///< Maximum tile side in pixels.
    static constexpr int half_size = size / 2;

    friend auto operator<=>(tile const&, tile const&) = default;

    friend auto hash_value(tile const& v) { return boost::pfr::hash_fields(v); }

    /// One raster pixel per tile pixel.
    static int zmax(geometry::raster::point ext)
    {
        auto n = std::max<>({ext.x(), ext.y(), size});
        return std::bit_width(unsigned((n - 1) / size));
    }

    static int zoom(geometry::raster::point ext, double scale)
    {
        int lim = zmax(ext);
        scale *= pow2(lim);
        for (int i = lim; i > 0; --i)
            if (scale >= pow2(i))
                return i;
        return 0;
    }

    /// Raster pixels per tile pixel.
    static int scale(geometry::raster::point ext, int zoom)
    {
        return pow2(zmax(ext) - zoom);
    }

    static std::generator<tile> all(geometry::raster::point ext, int zoom)
    {
        if (ext.x() <= 0 || ext.y() <= 0)
            co_return;
        int tl = size * scale(ext, zoom);
        auto nx = ext.x() / tl + (ext.x() % tl != 0);
        auto ny = ext.y() / tl + (ext.y() % tl != 0);
        for (int y{}; y < ny; ++y)
            for (int x{}; x < nx; ++x)
                co_yield {.z = zoom, .y = y, .x = x};
    }

    /// Clips the tile window to the raster extent.
    geometry::raster::box rect(geometry::raster::point ext) const
    {
        int tl = size * scale(ext, z);
        return {
            {std::clamp(x * tl, 0, ext.x()), std::clamp(y * tl, 0, ext.y())},
            {std::clamp((x + 1) * tl, 0, ext.x()),
             std::clamp((y + 1) * tl, 0, ext.y())}};
    }

    /// Maps tile pixels to raster pixels.
    auto affine(geometry::raster::point ext) const
    {
        auto [x, y, w, h] = geometry::xywh(rect(ext));
        auto px = scale(ext, z);
        auto nx = w / px + (w % px != 0);
        auto ny = h / px + (h % px != 0);
        return geometry::matrix{
            {{double(w) / nx, 0., double(x)},
             {0., double(h) / ny, double(y)},
             {0., 0., 1.}}};
    }
};

}  // namespace boat

template <>
struct std::hash<boat::tile> {
    static auto operator()(boat::tile const& v) { return hash_value(v); }
};

#endif  // BOAT_TILE_HPP
