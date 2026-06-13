// Andrew Naplavkov

#ifndef BOAT_SLIPPY_HPP
#define BOAT_SLIPPY_HPP

#include <boat/db/catalog.hpp>
#include <boat/detail/curl.hpp>
#include <boat/geometry/raster.hpp>
#include <boost/algorithm/string.hpp>

namespace boat::slippy {

class catalog : public db::catalog {
    inline static auto const err = std::runtime_error{"slippy"};

public:
    std::string user;
    std::string url;
    int epsg = 3857;  //< mercator only, e.g. 3395
    int zmax = 19;    //< 0-22

    std::vector<db::source> sources() override { return {}; }

    std::vector<db::layer> layers() override
    {
        return {{"", "_layer", "raster", true}};
    }

    db::table get_table(std::string_view, std::string_view) override
    {
        throw err;
    }

    db::rowset select(db::table const&, db::page const&) override { throw err; }

    db::rowset select(db::table const&, db::bbox const&) override { throw err; }

    void insert(db::table const&, db::rowset const&) override { throw err; }

    db::table create(db::table const&) override { throw err; }

    void drop(std::string_view, std::string_view) override {}

    db::raster get_raster(db::layer const&) override
    {
        static auto const lim =
            std::nexttoward(numbers::pi * numbers::earth::equatorial_radius, 0);
        static auto const mbr =
            geometry::cartesian::box{{-lim, -lim}, {lim, lim}};
        auto size = tile::size * pow2(zmax);
        auto mat = geometry::affine(size, size, mbr);
        return {
            .table_name{"_layer"},
            .column_name{"raster"},
            .bands{{"red", "byte"},
                   {"green", "byte"},
                   {"blue", "byte"},
                   {"alpha", "byte"}},
            .width = size,
            .height = size,
            .xorig = mat.a[0][2],
            .yorig = mat.a[1][2],
            .xscale = mat.a[0][0],
            .yscale = mat.a[1][1],
            .xskew = mat.a[0][1],
            .yskew = mat.a[1][0],
            .epsg = epsg,
        };
    }

    std::generator<std::pair<tile, gil::any_image>> read(
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
            auto [u, img] = queue.pop();
            co_yield {url_to_tile.at(u),
                      gil::read<boost::gil::rgba8_image_t>(img)};
        }
    }

    void write(db::raster const&, db::rect const&, gil::any_image_view) override
    {
        throw err;
    }
};

}  // namespace boat::slippy

#endif  // BOAT_SLIPPY_HPP
