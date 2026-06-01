// Andrew Naplavkov

#ifndef BOAT_TILE_HPP
#define BOAT_TILE_HPP

#include <boat/detail/utility.hpp>
#include <boost/pfr/ops_fields.hpp>
#include <boost/qvm/map_vec_mat.hpp>
#include <generator>

namespace boat {

struct tile {
    int z, y, x;

    static constexpr int size = 256;

    friend auto operator<=>(tile const&, tile const&) = default;

    friend auto hash_value(tile const& v) { return boost::pfr::hash_fields(v); }

    static int zmax(int width, int height)
    {
        auto px = static_cast<size_t>(std::max<>({width, height, size}));
        auto tl = static_cast<size_t>(std::bit_ceil(px) / size);
        return static_cast<int>(std::bit_width(tl)) - 1;
    }

    static int zoom(int width, int height, double scale)
    {
        int lim = zmax(width, height);
        scale *= pow2(lim);
        for (int i = lim; i > 0; --i)
            if (scale >= pow2(i))
                return i;
        return 0;
    }

    static int scale(int width, int height, int zoom)
    {
        return pow2(zmax(width, height) - zoom);
    }

    static std::generator<tile> level(int width, int height, int zoom)
    {
        int px = scale(width, height, zoom);
        int tl = size * px;
        int xmax = (width - 1) / tl;
        int ymax = (height - 1) / tl;
        for (int y{}; y <= ymax; ++y)
            for (int x{}; x <= xmax; ++x)
                co_yield {.z = zoom, .y = y, .x = x};
    }

    std::tuple<int, int, int, int> rect(int width, int height) const
    {
        int px = scale(width, height, z);
        int tl = size * px;
        int x1 = std::clamp(x * tl, 0, width);
        int y1 = std::clamp(y * tl, 0, height);
        int x2 = std::clamp((x + 1) * tl, 0, width);
        int y2 = std::clamp((y + 1) * tl, 0, height);
        return {x1, y1, x2 - x1, y2 - y1};
    }

    auto affine(int width, int height) const
    {
        double px = scale(width, height, z);
        double tl = size * px;
        return boost::qvm::translation_mat(boost::qvm::vec{{x * tl, y * tl}}) *
               boost::qvm::diag_mat(boost::qvm::vec{{px, px, 1.}});
    }
};

}  // namespace boat

template <>
struct std::hash<boat::tile> {
    static auto operator()(boat::tile const& v) { return hash_value(v); }
};

#endif  // BOAT_TILE_HPP
