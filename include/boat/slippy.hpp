// Andrew Naplavkov

#ifndef BOAT_SLIPPY_HPP
#define BOAT_SLIPPY_HPP

#include <boat/db/dal.hpp>
#include <boat/detail/curl.hpp>
#include <boat/geometry/raster.hpp>
#include <boost/algorithm/string.hpp>

namespace boat::slippy {

struct dal : db::dal {
    std::string user;
    std::string url;
    int epsg = 3857;  //< mercator only, e.g. 3395
    int zmax = 19;    //< 0-22

    std::vector<db::layer> vectors() override { return {}; }

    db::table get_table(std::string_view, std::string_view) override
    {
        return {};
    }

    db::rowset select(db::table const&, db::page const&) override
    {
        throw std::logic_error{"not implemented"};
    }

    db::rowset select(db::table const&, db::bbox const&) override
    {
        throw std::logic_error{"not implemented"};
    }

    void insert(db::table const&, db::rowset const&) override
    {
        throw std::logic_error{"not implemented"};
    }

    db::table create(db::table const&) override
    {
        throw std::logic_error{"not implemented"};
    }

    void drop(std::string_view, std::string_view) override {}

    std::vector<db::layer> rasters() override
    {
        return {{.table_name = "_layer", .column_name = "_raster"}};
    }

    db::raster get_raster(db::layer const& lyr) override
    {
        static auto const lim =
            std::nexttoward(numbers::pi * numbers::earth::equatorial_radius, 0);
        static auto const mbr =
            geometry::cartesian::box{{-lim, -lim}, {lim, lim}};
        auto size = tile::size * pow2(zmax);
        auto mat = geometry::affine(size, size, mbr);
        auto& a = mat.a;
        return {
            .schema_name = lyr.schema_name,
            .table_name = lyr.table_name,
            .column_name = lyr.column_name,
            .bands{{"red", "byte"}, {"green", "byte"}, {"blue", "byte"}},
            .width = size,
            .height = size,
            .xorig = a[0][2],
            .yorig = a[1][2],
            .xscale = a[0][0],
            .yscale = a[1][1],
            .xskew = a[0][1],
            .yskew = a[1][0],
            .epsg = epsg,
        };
    }

    std::generator<std::pair<tile, blob>> mosaic(  //
        db::raster r,
        std::vector<tile> ts) override
    {
        auto queue = curl{user};
        auto url_to_tile = std::unordered_map<std::string, tile>{};
        for (auto& t : ts) {
            auto u = url;
            u = boost::replace_first_copy(u, "{z}", to_chars(t.z));
            u = boost::replace_first_copy(u, "{y}", to_chars(t.y));
            u = boost::replace_first_copy(u, "{x}", to_chars(t.x));
            queue.push(u);
            url_to_tile.insert({u, t});
        }
        while (queue.size()) {
            auto [u, file] = queue.pop();
            co_yield {url_to_tile.at(u), std::move(file)};
        }
    }
};

}  // namespace boat::slippy

#endif  // BOAT_SLIPPY_HPP
