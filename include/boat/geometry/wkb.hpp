// Andrew Naplavkov

#ifndef BOAT_GEOMETRY_WKB_HPP
#define BOAT_GEOMETRY_WKB_HPP

#include <array>
#include <boat/blob.hpp>
#include <boat/geometry/detail/utility.hpp>

namespace boat {
namespace geometry::detail {

constexpr auto endians = std::array{std::endian::big, std::endian::little};
constexpr auto native = static_cast<uint8_t>(
    std::ranges::find(endians, std::endian::native) - endians.begin());
static_assert(native < endians.size());

constexpr auto read = overloaded{
    []<class T>(this auto&& self, blob_view& wkb, T& g, std::endian e) -> void {
        if constexpr (point<T>)
            g.x(get<double>(wkb, e)), g.y(get<double>(wkb, e));
        else
            for (uint32_t i{}, n = get<uint32_t>(wkb, e); i < n; ++i)
                if constexpr (multi<T>)
                    self(wkb, g.emplace_back());
                else if constexpr (polygon<T>)
                    self(wkb, i ? g.inners().emplace_back() : g.outer(), e);
                else
                    self(wkb, g.emplace_back(), e);
    },
    []<class T>(this auto&& self, blob_view& wkb, T& g) -> void {
        auto e = endians.at(get<uint8_t>(wkb));
        auto i = get<uint32_t>(wkb, e) - 1;
        if constexpr (dynamic<T>) {
            variant_emplace(g, i);
            std::visit([&](auto& g) { self(wkb, g, e); }, g);
        }
        else {
            check(i == variant_index_v<T>, "wkb");
            self(wkb, g = {}, e);
        }
    }};

constexpr auto write = overloaded{
    []<class T>(this auto&& self, T const& g, blob& wkb, auto) -> void {
        if constexpr (point<T>)
            wkb << g.x() << g.y();
        else if constexpr (polygon<T>) {
            wkb << static_cast<uint32_t>(1 + g.inners().size());
            self(g.outer(), wkb, std::ignore);
            for (auto& item : g.inners())
                self(item, wkb, std::ignore);
        }
        else {
            wkb << static_cast<uint32_t>(g.size());
            for (auto& item : g)
                if constexpr (multi<T>)
                    self(item, wkb);
                else
                    self(item, wkb, std::ignore);
        }
    },
    []<class T>(this auto&& self, T const& g, blob& wkb) -> void {
        if constexpr (dynamic<T>)
            std::visit([&](auto& g) { self(g, wkb); }, g);
        else {
            wkb << native << static_cast<uint32_t>(variant_index_v<T> + 1);
            self(g, wkb, std::ignore);
        }
    }};

}  // namespace geometry::detail

blob_view& operator>>(blob_view& wkb, geometry::ogc99 auto& geom)
{
    geometry::detail::read(wkb, geom);
    return wkb;
}

blob& operator<<(blob& wkb, geometry::ogc99 auto const& geom)
{
    geometry::detail::write(geom, wkb);
    return wkb;
}

}  // namespace boat

#endif  // BOAT_GEOMETRY_WKB_HPP
