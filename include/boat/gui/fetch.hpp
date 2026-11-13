// Andrew Naplavkov

#ifndef BOAT_GUI_FETCH_HPP
#define BOAT_GUI_FETCH_HPP

#include <boat/db/catalog.hpp>
#include <boat/geometry/raster.hpp>
#include <boat/geometry/wkb.hpp>
#include <boat/gui/caches/cache.hpp>
#include <boat/gui/detail/geometry.hpp>
#include <boat/gui/detail/tile.hpp>
#include <boat/gui/variant.hpp>
#include <stop_token>

namespace boat::gui {

struct layer_query {
    std::move_only_function<db::catalog&() const> catalog;
    db::layer layer;
    std::shared_ptr<caches::cache> cache;
    size_t cache_key;  ///< Unique within the cache.
    geometry::geographic::grid footprint;
    int limit = 512;
    std::stop_token stop;
};

namespace detail {

using caches::detail::get_or_invoke;

std::generator<variant> vectors(layer_query const& q)
{
    namespace bgi = boost::geometry::index;
    auto cells = std::max<>(1, int(std::sqrt(2 * q.limit)));
    auto tbl = get_or_invoke(q.cache.get(), q.cache_key, [&] {
        return q.catalog().get_table(q.layer.schema_name, q.layer.table_name);
    });
    auto& col = q.layer.column_name;
    auto it = std::ranges::find(tbl.columns, col, &db::column::column_name);
    check(it != tbl.columns.end(), col);
    auto crs = geometry::to_srs_variant(it->epsg, it->proj4);
    auto inv =
        geometry::transform(geometry::backward(geometry::transformation(crs)));
    auto done = bgi::rtree<geometry::cartesian::box, bgi::rstar<4>>{};
    for (auto& box : geometry::make_boxes(q.footprint, crs)) {
        if (q.stop.stop_requested())
            co_return;
        if (done.qbegin(bgi::contains(box)) != done.qend())
            continue;
        auto a = box.min_corner(), b = box.max_corner();
        auto vec = get_or_invoke(
            q.cache.get(),
            std::tuple{q.cache_key, a.x(), a.y(), b.x(), b.y(), q.limit},
            [&] {
                auto rs = q.catalog().select(tbl, {{col}, col, box, q.limit});
                auto ret = vector{};
                for (auto&& wkb : rs | db::view<blob>)
                    blob_view{wkb} >> ret.geoms.emplace_back();
                ret.partial = ret.geoms.size() >= q.limit;
                if (ret.partial)
                    ret.geoms = sample(std::move(ret.geoms), box, cells);
                ret.geoms =
                    inv(std::move(ret.geoms)).value_or(decltype(ret.geoms){});
                return ret;
            });
        if (!vec.partial)
            done.insert(box);
        if (!vec.geoms.empty())
            co_yield std::move(vec);
    }
}

std::generator<variant> rasters(layer_query const& q)
{
    auto& footprint = q.footprint;
    if (footprint.empty())
        co_return;
    auto r = get_or_invoke(q.cache.get(), q.cache_key, [&] {
        return q.catalog().get_raster(q.layer);
    });
    auto meters = std::next(footprint.begin(), footprint.size() > 1)->first;
    auto mat = affine(r);
    auto crs = geometry::to_srs_variant(r.epsg, r.proj4);
    auto uncached = std::vector<tile>{};
    for (auto& t :
         tiles(footprint.begin()->second, meters, r.extent, mat, crs)) {
        auto any =
            q.cache ? q.cache->get(std::pair{q.cache_key, t}) : std::any{};
        if (!any.has_value()) {
            uncached.push_back(t);
            continue;
        }
        co_yield raster{
            std::any_cast<decltype(raster::rgba)>(std::move(any)),
            mat * t.affine(r.extent),
            crs};
    }
    if (uncached.empty() || q.stop.stop_requested())
        co_return;
    for (auto [t, img] : q.catalog().read(r, std::move(uncached))) {
        auto rgba = gil::to<decltype(raster::rgba)>(const_view(img));
        if (q.cache)
            q.cache->put(std::pair{q.cache_key, t}, rgba);
        co_yield raster{std::move(rgba), mat * t.affine(r.extent), crs};
        if (q.stop.stop_requested())
            co_return;
    }
}

}  // namespace detail

inline std::generator<variant> fetch(layer_query const& q)
{
    return q.layer.raster ? detail::rasters(q) : detail::vectors(q);
}

}  // namespace boat::gui

#endif  // BOAT_GUI_FETCH_HPP
