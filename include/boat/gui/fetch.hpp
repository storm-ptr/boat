// Andrew Naplavkov

#ifndef BOAT_GUI_FETCH_HPP
#define BOAT_GUI_FETCH_HPP

#include <boat/gui/describe.hpp>
#include <boat/gui/detail/geometry.hpp>
#include <boat/gui/detail/tile.hpp>
#include <boat/gui/variant.hpp>

namespace boat::gui {

struct layer_query {
    std::shared_ptr<caches::cache> cache;
    size_t cache_key;  ///< Unique within the cache.
    db::layer layer;
    catalog_getter catalog;
    geometry::geographic::grid footprint;
    int limit = 512;
    std::stop_token stop;
};

namespace detail {

inline std::generator<variant> vectors(layer_query const& q)
{
    namespace bgi = boost::geometry::index;
    auto cells = std::max<>(1, int(std::sqrt(2 * q.limit)));
    auto tbl = get_table(q.cache.get(), q.cache_key, q.layer, q.catalog);
    auto& col = q.layer.column_name;
    auto& fld = db::find(tbl.columns, col);
    auto crs = geometry::make_srs_variant(fld.epsg, fld.proj4);
    auto inv =
        geometry::transform(geometry::backward(geometry::transformation(crs)));
    auto done = bgi::rtree<geometry::cartesian::box, bgi::rstar<4>>{};
    for (auto& box : geometry::make_boxes(q.footprint, crs)) {
        if (q.stop.stop_requested())
            co_return;
        if (done.qbegin(bgi::contains(box)) != done.qend())
            continue;
        auto a = box.min_corner(), b = box.max_corner();
        auto key = std::tuple{q.cache_key, a.x(), a.y(), b.x(), b.y(), q.limit};
        auto read = [&] {
            auto rs =
                q.catalog().select(tbl, {{col}, col, box, q.limit}, q.stop);
            if (q.stop.stop_requested())
                return vector{};
            auto ret = vector{.partial = rs.rows.size() >= q.limit};
            for (auto&& wkb : rs | db::view<blob>)
                blob_view{wkb} >> ret.geoms.emplace_back();
            if (ret.partial)
                ret.geoms = sample(std::move(ret.geoms), box, cells);
            ret.geoms =
                inv(std::move(ret.geoms)).value_or(decltype(ret.geoms){});
            return ret;
        };
        auto vec = get_or_invoke(q.cache.get(), key, q.stop, read);
        if (!vec.partial)
            done.insert(box);
        if (!vec.geoms.empty() && !q.stop.stop_requested())
            co_yield std::move(vec);
    }
}

inline std::generator<variant> rasters(layer_query const& q)
{
    auto& grid = q.footprint;
    if (grid.empty())
        co_return;
    auto r = get_raster(q.cache.get(), q.cache_key, q.layer, q.catalog);
    auto meters = std::next(grid.begin(), grid.size() > 1)->first;
    auto mat = affine(r);
    auto crs = geometry::make_srs_variant(r.epsg, r.proj4);
    auto uncached = std::vector<tile>{};
    for (auto& t : tiles(grid.begin()->second, meters, r.extent, mat, crs)) {
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

/// Raster queries use larger boxes. Smaller vector query boxes spread sampled
/// features more evenly across the view.
inline std::generator<variant> fetch(layer_query const& q)
{
    return q.layer.raster ? detail::rasters(q) : detail::vectors(q);
}

}  // namespace boat::gui

#endif  // BOAT_GUI_FETCH_HPP
