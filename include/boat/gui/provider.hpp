// Andrew Naplavkov

#ifndef BOAT_GUI_PROVIDER_HPP
#define BOAT_GUI_PROVIDER_HPP

#include <boat/db/dal.hpp>
#include <boat/gui/caches/cache.hpp>
#include <boat/gui/detail/geometry.hpp>
#include <boat/gui/detail/tile.hpp>
#include <boat/gui/feature.hpp>
#include <generator>
#include <random>

namespace boat::gui {

struct provider {
    std::move_only_function<db::dal&()> dal;
    db::layer layer;
    std::shared_ptr<caches::cache> cache;
    size_t key;
    geometry::geographic::grid grid;

    std::generator<feature> features()
    {
        co_yield std::ranges::elements_of(shapes());
        co_yield std::ranges::elements_of(images());
    }

private:
    std::generator<shape> shapes()
    {
        namespace bgi = boost::geometry::index;
        auto tbl =
            get_or_invoke(cache.get(), std::tuple{key, "get_table"}, [&] {
                return dal().get_table(layer.schema_name, layer.table_name);
            });
        if (tbl.columns.empty())
            co_return;
        auto& col = layer.column_name;
        auto it = std::ranges::find(tbl.columns, col, &db::column::column_name);
        check(it != tbl.columns.end(), col);
        auto voids = bgi::rtree<geometry::cartesian::box, bgi::rstar<4>>{};
        auto yields = std::unordered_set<blob>{};
        auto gen = std::mt19937{std::random_device()()};
        for (auto& box : boxes(grid, epsg(it->epsg))) {
            if (bgi::qbegin(voids, bgi::contains(box)) != bgi::qend(voids))
                continue;
            auto a = box.min_corner(), b = box.max_corner();
            auto wkbs = get_or_invoke(
                cache.get(),
                std::tuple{key, "select", a.x(), a.y(), b.x(), b.y()},
                [&] {
                    auto ret = std::vector<blob>{};
                    auto rows = dal().select(
                        tbl,
                        db::bbox{{col}, col, a.x(), a.y(), b.x(), b.y(), 1024});
                    std::ranges::sample(  //
                        rows | db::view<blob>,
                        std::back_inserter(ret),
                        64,
                        gen);
                    return ret;
                });
            if (wkbs.empty())
                voids.insert(box);
            else
                for (auto& wkb : wkbs)
                    if (yields.insert(wkb).second)
                        co_yield shape{std::move(wkb), it->epsg};
        }
    }

    std::generator<image> images()
    {
        auto r = get_or_invoke(cache.get(), std::tuple{key, "get_raster"}, [&] {
            return dal().get_raster(layer);
        });
        if (r.bands.empty())
            co_return;
        auto affine = geometry::matrix{{
            {r.xscale, r.xskew, r.xorig},
            {r.yskew, r.yscale, r.yorig},
            {0., 0., 1.},
        }};
        auto sys = epsg(r.epsg);
        auto uncached = std::vector<tile>{};
        for (auto& t : tiles(r.width, r.height, affine, sys, grid)) {
            auto file = cache ? cache->get(std::tuple{key, t}) : std::any{};
            if (!file.has_value()) {
                uncached.push_back(t);
                continue;
            }
            co_yield {std::any_cast<blob>(std::move(file)),
                      affine * t.affine(r.width, r.height),
                      r.epsg};
        }
        for (auto [t, file] : dal().mosaic(r, std::move(uncached))) {
            if (cache)
                cache->put(std::tuple{key, t}, file);
            co_yield {std::any_cast<blob>(std::move(file)),
                      affine * t.affine(r.width, r.height),
                      r.epsg};
        }
    }
};

}  // namespace boat::gui

#endif  // BOAT_GUI_PROVIDER_HPP
