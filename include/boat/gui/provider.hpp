// Andrew Naplavkov

#ifndef BOAT_GUI_PROVIDER_HPP
#define BOAT_GUI_PROVIDER_HPP

#include <boat/db/catalog.hpp>
#include <boat/gui/caches/cache.hpp>
#include <boat/gui/detail/geometry.hpp>
#include <boat/gui/detail/tile.hpp>
#include <boat/gui/variant.hpp>
#include <random>

namespace boat::gui {

struct provider {
    std::move_only_function<db::catalog&()> catalog;
    db::layer layer;
    std::shared_ptr<caches::cache> cache;
    size_t key;
    geometry::geographic::grid grid;

    std::generator<variant> variants()
    {
        if (layer.raster)
            co_yield std::ranges::elements_of(rasters());
        else
            co_yield std::ranges::elements_of(vectors());
    }

private:
    std::generator<vector> vectors()
    {
        namespace bgi = boost::geometry::index;
        auto tbl =
            get_or_invoke(cache.get(), std::tuple{key, "get_table"}, [&] {
                return catalog().get_table(layer.schema_name, layer.table_name);
            });
        auto& col = layer.column_name;
        auto it = std::ranges::find(tbl.columns, col, &db::column::column_name);
        check(it != tbl.columns.end(), col);
        auto crs = geometry::srs::epsg(it->epsg);
        auto voids = bgi::rtree<geometry::cartesian::box, bgi::rstar<4>>{};
        auto done = std::unordered_set<blob>{};
        auto gen = std::mt19937{std::random_device()()};
        for (auto& box : boxes(grid, crs)) {
            if (bgi::qbegin(voids, bgi::contains(box)) != bgi::qend(voids))
                continue;
            auto a = box.min_corner(), b = box.max_corner();
            auto wkb = get_or_invoke(
                cache.get(),
                std::tuple{key, "select", a.x(), a.y(), b.x(), b.y()},
                [&] {
                    auto ret = std::vector<blob>{};
                    auto rs = catalog().select(
                        tbl,
                        db::bbox{{col}, col, a.x(), a.y(), b.x(), b.y(), 1024});
                    std::ranges::sample(  //
                        rs | db::view<blob>,
                        std::back_inserter(ret),
                        64,
                        gen);
                    return ret;
                });
            if (wkb.empty())
                voids.insert(box);
            std::erase_if(wkb, [&](auto& v) { return !done.insert(v).second; });
            if (!wkb.empty())
                co_yield {std::move(wkb), crs};
        }
    }

    std::generator<raster<>> rasters()
    {
        auto r = get_or_invoke(cache.get(), std::tuple{key, "get_raster"}, [&] {
            return catalog().get_raster(layer);
        });
        auto affine = geometry::matrix{{
            {r.xscale, r.xskew, r.xorig},
            {r.yskew, r.yscale, r.yorig},
            {0., 0., 1.},
        }};
        auto crs = geometry::srs::epsg(r.epsg);
        auto uncached = std::vector<tile>{};
        for (auto& t : tiles(grid, r.width, r.height, affine, crs)) {
            auto data = cache ? cache->get(std::tuple{key, t}) : std::any{};
            if (!data.has_value()) {
                uncached.push_back(t);
                continue;
            }
            co_yield {std::any_cast<blob>(std::move(data)),
                      affine * t.affine(r.width, r.height),
                      crs};
        }
        for (auto [t, data] : catalog().mosaic(r, std::move(uncached))) {
            if (cache)
                cache->put(std::tuple{key, t}, data);
            co_yield {std::any_cast<blob>(std::move(data)),
                      affine * t.affine(r.width, r.height),
                      crs};
        }
    }
};

}  // namespace boat::gui

#endif  // BOAT_GUI_PROVIDER_HPP
