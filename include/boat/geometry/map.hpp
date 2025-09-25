// Andrew Naplavkov

#ifndef BOAT_GEOMETRY_MAP_HPP
#define BOAT_GEOMETRY_MAP_HPP

#include <boat/geometry/transform.hpp>

namespace boat::geometry {

inline std::optional<cartesian::box> forward(projection auto const& pj,
                                             geographic::point const& center,
                                             double resolution,
                                             int width,
                                             int height)
{
    auto fwd = transformer(forwarder(pj));
    auto xy = fwd(center);
    if (!xy)
        return std::nullopt;
    auto c = cartesian::point{xy->x(), xy->y()};
    auto scale = 0.;
    for (auto ll : buffer(resolution, 4)(center).outer())
        if (xy = fwd(ll))
            scale = std::max<>(scale,
                               boost::geometry::distance(
                                   c, cartesian::point{xy->x(), xy->y()}));
    if (!scale)
        return std::nullopt;
    auto mbr =
        cartesian::box{{-width / 2., -height / 2.}, {width / 2., height / 2.}};
    boost::geometry::multiply_value(mbr.min_corner(), scale);
    boost::geometry::multiply_value(mbr.max_corner(), scale);
    boost::geometry::add_point(mbr.min_corner(), c);
    boost::geometry::add_point(mbr.max_corner(), c);
    return std::optional{std::move(mbr)};
}

geographic::grid inverse(projection auto const& pj,
                         cartesian::box const& mbr,
                         size_t num_points)
{
    auto lls = geographic::multi_point{};
    for (auto xy : box_fibonacci(mbr, num_points))
        if (!pj.inverse(xy, lls.emplace_back()))
            lls.pop_back();
    auto accept = [&](geographic::point const& ll) {
        auto xy = cartesian::point{};
        return pj.forward(ll, xy) && boost::geometry::within(xy, mbr);
    };
    auto ret = geographic::grid{};
    for (auto z : std::views::iota(0)) {
        auto fib = geographic_fibonacci{static_cast<size_t>(std::pow(2, z))};
        auto indices = std::unordered_set<size_t>{};
        for (auto& ll : lls)
            for (auto i : fib.nearests(ll, accept)) {
                if (!indices.insert(i).second)
                    break;
                if (indices.size() > lls.size())
                    return ret;
            }
        auto& lvl = ret[root_geoid_area / std::sqrt(fib.num_points)];
        lvl.resize(indices.size());
        for (auto [i, j] : indices | std::views::enumerate)
            lvl[i] = fib[j];
    }
    return ret;
}

}  // namespace boat::geometry

#endif  // BOAT_GEOMETRY_MAP_HPP
