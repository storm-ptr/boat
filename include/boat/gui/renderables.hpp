// Andrew Naplavkov

#ifndef BOAT_GUI_RENDERABLES_HPP
#define BOAT_GUI_RENDERABLES_HPP

#include <boat/db/catalog.hpp>
#include <boat/geometry/wkb.hpp>
#include <boat/gui/caches/cache.hpp>
#include <boat/gui/detail/geometry.hpp>
#include <boat/gui/detail/tile.hpp>
#include <boat/gui/utility.hpp>
#include <boat/gui/variant.hpp>
#include <stop_token>

namespace boat::gui {

struct render_params {
    std::move_only_function<db::catalog&() const> catalog;
    db::layer layer;
    std::shared_ptr<caches::cache> cache;
    size_t cache_key;  ///< Layer data revision.
    geometry::geographic::grid query_grid;
    int query_limit = 512;
    std::stop_token stop;
};

namespace detail {

using caches::detail::get_or_invoke;

std::generator<variant> vectors(auto const& p)
{
    namespace bgi = boost::geometry::index;
    auto cells = std::max<>(1, int(std::sqrt(2 * p.query_limit)));
    auto tbl = get_or_invoke(p.cache.get(), p.cache_key, [&] {
        return p.catalog().get_table(p.layer.schema_name, p.layer.table_name);
    });
    auto& col = p.layer.column_name;
    auto it = std::ranges::find(tbl.columns, col, &db::column::column_name);
    check(it != tbl.columns.end(), col);
    auto crs = to_srs_variant(*it);
    auto inv = geometry::transform(
        geometry::srs_inverse(geometry::transformation(crs)));
    auto done = bgi::rtree<geometry::cartesian::box, bgi::rstar<4>>{};
    for (auto& box : query_boxes(p.query_grid, crs)) {
        if (p.stop.stop_requested())
            co_return;
        if (done.qbegin(bgi::contains(box)) != done.qend())
            continue;
        auto a = box.min_corner(), b = box.max_corner();
        auto vec = get_or_invoke(
            p.cache.get(),
            std::tuple{p.cache_key, a.x(), a.y(), b.x(), b.y(), p.query_limit},
            [&] {
                auto rs = p.catalog().select(
                    tbl,
                    {{col}, col, a.x(), a.y(), b.x(), b.y(), p.query_limit});
                auto ret = vector{};
                for (auto&& wkb : rs | db::view<blob>)
                    blob_view{wkb} >> ret.geoms.emplace_back();
                ret.partial = ret.geoms.size() >= p.query_limit;
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

std::generator<variant> rasters(auto const& p)
{
    auto& grid = p.query_grid;
    if (grid.empty())
        co_return;
    auto r = get_or_invoke(p.cache.get(), p.cache_key, [&] {
        return p.catalog().get_raster(p.layer);
    });
    auto meters = std::next(grid.begin(), grid.size() > 1)->first;
    auto mat = affine(r);
    auto crs = to_srs_variant(r);
    auto uncached = std::vector<tile>{};
    for (auto& t :
         tiles(grid.begin()->second, meters, r.width, r.height, mat, crs)) {
        auto any =
            p.cache ? p.cache->get(std::pair{p.cache_key, t}) : std::any{};
        if (!any.has_value()) {
            uncached.push_back(t);
            continue;
        }
        co_yield raster{
            std::any_cast<decltype(raster::rgba)>(std::move(any)),
            mat * t.affine(r.width, r.height),
            crs};
    }
    if (uncached.empty() || p.stop.stop_requested())
        co_return;
    for (auto [t, img] : p.catalog().read(r, std::move(uncached))) {
        auto rgba = gil::to<decltype(raster::rgba)>(const_view(img));
        if (p.cache)
            p.cache->put(std::pair{p.cache_key, t}, rgba);
        co_yield raster{
            std::move(rgba), mat * t.affine(r.width, r.height), crs};
        if (p.stop.stop_requested())
            co_return;
    }
}

}  // namespace detail

inline auto renderables(render_params const& p)
{
    return p.layer.raster ? detail::rasters(p) : detail::vectors(p);
}

}  // namespace boat::gui

#endif  // BOAT_GUI_RENDERABLES_HPP
