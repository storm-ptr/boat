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
    std::generator<geometry::geographic::geometry_collection> vectors()
    {
        namespace bgi = boost::geometry::index;
        auto tbl = get_or_invoke(cache.get(), key, [&] {
            return catalog().get_table(layer.schema_name, layer.table_name);
        });
        auto& col = layer.column_name;
        auto it = std::ranges::find(tbl.columns, col, &db::column::column_name);
        check(it != tbl.columns.end(), col);
        auto crs = geometry::srs::epsg(it->epsg);
        auto voids = bgi::rtree<geometry::cartesian::box, bgi::rstar<4>>{};
        auto gen = std::mt19937{std::random_device()()};
        for (auto& box : boxes(grid, crs)) {
            if (bgi::qbegin(voids, bgi::contains(box)) != bgi::qend(voids))
                continue;
            auto a = box.min_corner(), b = box.max_corner();
            auto coll = get_or_invoke(
                cache.get(), std::tuple{key, a.x(), a.y(), b.x(), b.y()}, [&] {
                    auto rs = catalog().select(
                        tbl,
                        db::bbox{{col}, col, a.x(), a.y(), b.x(), b.y(), 1024});
                    auto wkb = std::vector<blob>{};
                    std::ranges::sample(
                        rs | db::view<blob>, std::back_inserter(wkb), 64, gen);
                    auto inv = geometry::transform(
                        geometry::srs_inverse(geometry::transformation(crs)));
                    auto ret = geometry::geographic::geometry_collection{};
                    for (blob_view item : wkb) {
                        auto g1 = geometry::geographic::variant{};
                        item >> g1;
                        if (auto g2 = inv(g1))
                            ret.push_back(*g2);
                    }
                    return ret;
                });
            if (coll.empty())
                voids.insert(box);
            else
                co_yield std::move(coll);
        }
    }

    std::generator<raster> rasters()
    {
        auto r = get_or_invoke(
            cache.get(), key, [&] { return catalog().get_raster(layer); });
        auto affine = geometry::matrix{{
            {r.xscale, r.xskew, r.xorig},
            {r.yskew, r.yscale, r.yorig},
            {0., 0., 1.},
        }};
        auto crs = geometry::srs::epsg(r.epsg);
        auto uncached = std::vector<tile>{};
        for (auto& t : tiles(grid, r.width, r.height, affine, crs)) {
            auto any = cache ? cache->get(std::tuple{key, t}) : std::any{};
            if (!any.has_value()) {
                uncached.push_back(t);
                continue;
            }
            auto rgba =
                std::any_cast<boost::gil::rgba8_image_t>(std::move(any));
            co_yield {
                std::move(rgba), affine * t.affine(r.width, r.height), crs};
        }
        for (auto [t, img] : catalog().read(r, std::move(uncached))) {
            auto rgba = gil::to<boost::gil::rgba8_image_t>(const_view(img));
            if (cache)
                cache->put(std::tuple{key, t}, rgba);
            co_yield {
                std::move(rgba), affine * t.affine(r.width, r.height), crs};
        }
    }
};

}  // namespace boat::gui

#endif  // BOAT_GUI_PROVIDER_HPP
