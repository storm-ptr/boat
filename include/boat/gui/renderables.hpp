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

namespace boat::gui {

struct render_params {
    std::move_only_function<db::catalog&() const> catalog;
    db::layer layer;
    std::shared_ptr<caches::cache> cache;
    size_t cache_key;                   ///< Change when layer data changes.
    geometry::geographic::grid filter;  ///< Sample points by spacing in metres.
    int query_limit = 512;
};

namespace detail {

auto key(render_params const& p, auto const&... args)
{
    return std::tuple{p.cache_key, args...};
}

inline std::generator<variant> vectors(render_params const& p)
{
    namespace bgi = boost::geometry::index;
    auto cells_per_side =
        std::max<>(1, static_cast<int>(std::sqrt(p.query_limit)));
    auto tbl = caches::detail::get_or_invoke(p.cache.get(), key(p), [&] {
        return p.catalog().get_table(p.layer.schema_name, p.layer.table_name);
    });
    auto& col = p.layer.column_name;
    auto it = std::ranges::find(tbl.columns, col, &db::column::column_name);
    check(it != tbl.columns.end(), col);
    auto crs = to_srs_variant(*it);
    auto inv = geometry::transform(
        geometry::srs_inverse(geometry::transformation(crs)));
    auto done = bgi::rtree<geometry::cartesian::box, bgi::rstar<4>>{};
    for (auto& box : boxes(p.filter, crs)) {
        if (bgi::qbegin(done, bgi::contains(box)) != bgi::qend(done))
            continue;
        auto a = box.min_corner(), b = box.max_corner();
        auto vec = caches::detail::get_or_invoke(
            p.cache.get(),
            key(p, a.x(), a.y(), b.x(), b.y(), p.query_limit),
            [&] {
                auto rs = p.catalog().select(
                    tbl,
                    {{col}, col, a.x(), a.y(), b.x(), b.y(), p.query_limit});
                auto ret = vector{};
                for (auto&& wkb : rs | db::view<blob>)
                    blob_view{wkb} >> ret.geoms.emplace_back();
                ret.partial = ret.geoms.size() >= p.query_limit;
                if (ret.partial)
                    ret.geoms =
                        sample(std::move(ret.geoms), box, cells_per_side);
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

inline std::generator<variant> rasters(render_params const& p)
{
    auto r = caches::detail::get_or_invoke(
        p.cache.get(), key(p), [&] { return p.catalog().get_raster(p.layer); });
    auto mat = affine(r);
    auto crs = to_srs_variant(r);
    auto uncached = std::vector<tile>{};
    for (auto& t : tiles(p.filter, r.width, r.height, mat, crs)) {
        auto any = p.cache ? p.cache->get(key(p, t)) : std::any{};
        if (!any.has_value()) {
            uncached.push_back(t);
            continue;
        }
        auto rgba = std::any_cast<boost::gil::rgba8_image_t>(std::move(any));
        co_yield raster{
            std::move(rgba), mat * t.affine(r.width, r.height), crs};
    }
    if (uncached.empty())
        co_return;
    for (auto [t, img] : p.catalog().read(r, std::move(uncached))) {
        auto rgba = gil::to<boost::gil::rgba8_image_t>(const_view(img));
        if (p.cache)
            p.cache->put(key(p, t), rgba);
        co_yield raster{
            std::move(rgba), mat * t.affine(r.width, r.height), crs};
    }
}

}  // namespace detail

inline auto renderables(render_params const& p)
{
    return p.layer.raster ? detail::rasters(p) : detail::vectors(p);
}

}  // namespace boat::gui

#endif  // BOAT_GUI_RENDERABLES_HPP
