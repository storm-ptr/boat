// Andrew Naplavkov

#ifndef BOAT_GUI_PROVIDER_HPP
#define BOAT_GUI_PROVIDER_HPP

#include <boat/db/adapted/geometry.hpp>
#include <boat/db/catalog.hpp>
#include <boat/geometry/wkb.hpp>
#include <boat/gui/caches/cache.hpp>
#include <boat/gui/detail/geometry.hpp>
#include <boat/gui/detail/tile.hpp>
#include <boat/gui/utility.hpp>
#include <boat/gui/variant.hpp>

namespace boat::gui {

struct provider {
    std::move_only_function<db::catalog&()> catalog;
    db::layer layer;
    std::shared_ptr<caches::cache> cache;
    size_t cache_key;
    geometry::geographic::grid filter;
    int query_limit = 512;

    auto renderables() { return layer.raster ? rasters() : vectors(); }

private:
    std::generator<variant> vectors()
    {
        namespace bgi = boost::geometry::index;
        auto cells_per_side =
            std::max(1, static_cast<int>(std::sqrt(query_limit)));
        auto tbl = get_or_invoke(cache.get(), cache_key, [&] {
            return catalog().get_table(layer.schema_name, layer.table_name);
        });
        auto& col = layer.column_name;
        auto it = std::ranges::find(tbl.columns, col, &db::column::column_name);
        check(it != tbl.columns.end(), col);
        auto crs = geometry::to_srs_variant(*it);
        auto inv = geometry::transform(
            geometry::srs_inverse(geometry::transformation(crs)));
        auto done = bgi::rtree<geometry::cartesian::box, bgi::rstar<4>>{};
        for (auto& box : boxes(filter, crs)) {
            if (bgi::qbegin(done, bgi::contains(box)) != bgi::qend(done))
                continue;
            auto a = box.min_corner(), b = box.max_corner();
            auto vec = get_or_invoke(  //
                cache.get(),
                std::tuple{cache_key, a.x(), a.y(), b.x(), b.y()},
                [&] {
                    auto rs = catalog().select(
                        tbl,
                        {{col}, col, a.x(), a.y(), b.x(), b.y(), query_limit});
                    auto ret = vector{};
                    for (auto&& wkb : rs | db::view<blob>)
                        blob_view{wkb} >> ret.geoms.emplace_back();
                    ret.partial = ret.geoms.size() >= query_limit;
                    if (ret.partial)
                        ret.geoms =
                            sample(std::move(ret.geoms), box, cells_per_side);
                    ret.geoms = inv(std::move(ret.geoms))
                                    .value_or(decltype(ret.geoms){});
                    return ret;
                });
            if (!vec.partial)
                done.insert(box);
            if (!vec.geoms.empty())
                co_yield std::move(vec);
        }
    }

    std::generator<variant> rasters()
    {
        auto r = get_or_invoke(cache.get(), cache_key, [&] {
            return catalog().get_raster(layer);
        });
        auto affine = db::affine(r);
        auto crs = geometry::to_srs_variant(r);
        auto uncached = std::vector<tile>{};
        for (auto& t : tiles(filter, r.width, r.height, affine, crs)) {
            auto any =
                cache ? cache->get(std::tuple{cache_key, t}) : std::any{};
            if (!any.has_value()) {
                uncached.push_back(t);
                continue;
            }
            auto rgba =
                std::any_cast<boost::gil::rgba8_image_t>(std::move(any));
            co_yield raster{
                std::move(rgba), affine * t.affine(r.width, r.height), crs};
        }
        if (uncached.empty())
            co_return;
        for (auto [t, img] : catalog().read(r, std::move(uncached))) {
            auto rgba = gil::to<boost::gil::rgba8_image_t>(const_view(img));
            if (cache)
                cache->put(std::tuple{cache_key, t}, rgba);
            co_yield raster{
                std::move(rgba), affine * t.affine(r.width, r.height), crs};
        }
    }
};

}  // namespace boat::gui

#endif  // BOAT_GUI_PROVIDER_HPP
