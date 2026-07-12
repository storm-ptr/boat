// Andrew Naplavkov

#ifndef BOAT_SLIPPY_HPP
#define BOAT_SLIPPY_HPP

#include <boat/db/catalog.hpp>
#include <boat/detail/curl.hpp>
#include <boat/geometry/raster.hpp>
#include <boost/algorithm/string.hpp>

namespace boat::slippy {

class catalog : public db::catalog {
    inline static auto err = std::logic_error{"slippy"};

public:
    std::string user;
    std::string url;
    int epsg = 3857;  //< or 3395
    int zmax = 19;

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

    void insert(  //
        db::table const&,
        db::rowset const&,
        std::stop_token = {}) override
    {
        throw err;
    }

    db::table create(db::table const&) override { throw err; }

    void drop(std::string_view, std::string_view) override {}

    db::raster get_raster(db::layer const&) override
    {
        auto lim =
            std::nexttoward(numbers::pi * numbers::earth::equatorial_radius, 0);
        auto mbr = geometry::cartesian::box{{-lim, -lim}, {lim, lim}};
        auto sz = tile::size * pow2(zmax);
        auto [a] = geometry::affine(sz, sz, mbr);
        return {
            .table_name{"_layer"},
            .column_name{"raster"},
            .bands{{"red", "byte"},
                   {"green", "byte"},
                   {"blue", "byte"},
                   {"alpha", "byte"}},
            .width = sz,
            .height = sz,
            .xorig = a[0][2],
            .yorig = a[1][2],
            .xscale = a[0][0],
            .yscale = a[1][1],
            .xskew = a[0][1],
            .yskew = a[1][0],
            .srid = epsg,
            .epsg = epsg,
        };
    }

    std::generator<std::pair<tile, gil::any_image>> read(
        db::raster,
        std::vector<tile> ts) override
    {
        auto q = curl{user};
        auto m = std::map<std::string, tile>{};
        for (auto& t : ts) {
            auto u = url;
            u = boost::replace_first_copy(u, "{z}", to_chars(t.z));
            u = boost::replace_first_copy(u, "{y}", to_chars(t.y));
            u = boost::replace_first_copy(u, "{x}", to_chars(t.x));
            q.push(u);
            m.insert({u, t});
        }
        while (q.size()) {
            auto [u, img] = q.pop();
            co_yield {m.at(u), gil::read<boost::gil::rgba8_image_t>(img)};
        }
    }

    void write(db::raster const&, db::rect const&, gil::any_image_view) override
    {
        throw err;
    }

    void set_autocommit(bool) override { throw err; }

    void commit() override { throw err; }
};

}  // namespace boat::slippy

#endif  // BOAT_SLIPPY_HPP
