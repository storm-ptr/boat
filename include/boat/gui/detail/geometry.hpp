// Andrew Naplavkov

#ifndef BOAT_GUI_GEOMETRY_HPP
#define BOAT_GUI_GEOMETRY_HPP

#include <boat/geometry/algorithm.hpp>
#include <boat/geometry/transform.hpp>
#include <boat/geometry/wkb.hpp>

namespace boat::gui {

auto bidirectional(  //
    geometry::matrix const& affine1,
    geometry::srs_spec auto const& crs1,
    geometry::matrix const& affine2,
    geometry::srs_spec auto const& crs2)
{
    auto tf = geometry::transformation(crs1, crs2);
    return std::pair{
        geometry::transform(  //
            geometry::mat_forward(affine1),
            geometry::srs_forward(tf),
            geometry::mat_inverse(affine2)),
        geometry::transform(  //
            geometry::mat_forward(affine2),
            geometry::srs_inverse(tf),
            geometry::mat_inverse(affine1)),
    };
}

auto boxes(  //
    geometry::geographic::grid const& grid,
    geometry::srs_spec auto const& crs)
{
    auto ret = std::vector<geometry::cartesian::box>{};
    auto fwd = geometry::transform(
        geometry::srs_forward(geometry::transformation(crs)));
    auto add = [&](geometry::geographic::box const& ll) {
        auto a = ll.min_corner(), b = ll.max_corner();
        if (a.x() <= -180. || b.x() >= 180. || a.y() <= -90. || b.y() >= 90.)
            return;
        if (auto xy = fwd(ll).transform(geometry::cartesian{}))
            ret.push_back(*xy);
    };
    for (auto& lvl : grid | std::views::reverse) {
        auto d = lvl.first * numbers::inv_sqrt_2;
        if (d >= numbers::earth::sqrt_area / 4)
            continue;
        for (auto& p : lvl.second) {
            auto x = p.x(), y = p.y();
            auto dx = d * geometry::meter(y), dy = d * geometry::meter();
            add({{x, y}, {x + dx, y + dy}});
            add({{x - dx, y}, {x, y + dy}});
            add({{x, y - dy}, {x + dx, y}});
            add({{x - dx, y - dy}, {x, y}});
        }
    }
    return ret;
}

inline auto multi_point(int width, int height)
{
    auto size = std::max<>(width, height);
    auto num_border_points =
        static_cast<int>(4 * std::max<>(std::sqrt(size), 1.));
    auto num_area_points = std::max<>(size / 4, 1);
    auto mbr = geometry::geographic::box{{}, {width * 1., height * 1.}};
    auto ret = geometry::box_border_interpolate(mbr, num_border_points);
    ret.append_range(geometry::box_area_interpolate(mbr, num_area_points));
    return ret;
}

auto sample(  //
    std::ranges::input_range auto&& wkbs,
    geometry::cartesian::box const& box,
    size_t limit)
{
    namespace geo = geometry;
    using collection = geo::geographic::geometry_collection;
    auto cells = std::array<collection, 64>{};
    auto a = box.min_corner(), b = box.max_corner();
    for (auto&& wkb : wkbs) {
        auto geom = geo::geographic::variant{};
        blob_view{wkb} >> geom;
        auto clip = geo::cartesian::box{};
        if (!boost::geometry::intersection(
                box, geo::cartesian{}(geo::minmax(geom)), clip))
            continue;
        auto p = boost::geometry::return_centroid<geo::cartesian::point>(clip);
        auto x = std::clamp(int(8 * (p.x() - a.x()) / (b.x() - a.x())), 0, 7);
        auto y = std::clamp(int(8 * (p.y() - a.y()) / (b.y() - a.y())), 0, 7);
        cells[y * 8 + x].push_back(std::move(geom));
    }
    auto ret = collection{};
    auto max_size =
        std::ranges::max_element(cells, {}, &collection::size)->size();
    for (auto i : std::views::iota(0uz, max_size))
        for (auto& cell : cells)
            if (i < cell.size()) {
                if (ret.size() == limit)
                    return ret;
                ret.push_back(std::move(cell[i]));
            }
    return ret;
}

}  // namespace boat::gui

#endif  // BOAT_GUI_GEOMETRY_HPP
