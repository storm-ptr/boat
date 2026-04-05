// Andrew Naplavkov

#ifndef BOAT_TILE_HPP
#define BOAT_TILE_HPP

#include <boat/detail/utility.hpp>
#include <boost/pfr/ops_fields.hpp>
#include <boost/qvm/map_vec_mat.hpp>

namespace boat {

struct tile {
    int z;
    int y;
    int x;

    static constexpr int size = 256;

    friend auto operator<=>(tile const&, tile const&) = default;

    friend size_t hash_value(tile const& that)
    {
        return boost::pfr::hash_fields(that);
    }

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

    std::pair<int, int> min_corner(int width, int height) const
    {
        int px = scale(width, height, z);
        int tl = size * px;
        return {std::clamp(x * tl, 0, width), std::clamp(y * tl, 0, height)};
    }

    std::pair<int, int> max_corner(int width, int height) const
    {
        return tile{.z = z, .y = y + 1, .x = x + 1}.min_corner(width, height);
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
    static size_t operator()(boat::tile const& that)
    {
        return hash_value(that);
    }
};

#endif  // BOAT_TILE_HPP
