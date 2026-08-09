// Andrew Naplavkov

#include <boat/catalogs.hpp>
#include <boat/geometry/wkb.hpp>
#include <boost/geometry/views/box_view.hpp>
#include "catalog.h"

namespace {

auto const column_name = "box";
auto const err = std::logic_error{"echo"};
auto const table_name = "echo";

class echo : public boat::db::catalog {
public:
    std::vector<boat::db::source> sources() override { return {}; }

    std::vector<boat::db::layer> layers() override
    {
        return {{.table_name{table_name}, .column_name{column_name}}};
    }

    boat::db::table get_table(std::string_view, std::string_view) override
    {
        return {
            .table_name{table_name},
            .columns{
                {.kind{"geometry"}, .column_name{column_name}, .epsg = 4326}},
        };
    }

    boat::db::rowset select(boat::db::table const&,
                            boat::db::page const&) override
    {
        throw err;
    }

    boat::db::rowset select(  //
        boat::db::table const&,
        boat::db::bbox const& rq) override
    {
        auto ret = boat::db::rowset{.columns{column_name}};
        auto mbr = boat::geometry::cartesian::box{
            {rq.xmin, rq.ymin},
            {rq.xmax, rq.ymax},
        };
        auto points = boat::geometry::box_border_interpolate(mbr, 92);
        auto wkb = boat::blob{} << points;
        ret.rows.push_back({std::move(wkb)});
        return ret;
    }

    void insert(  //
        boat::db::table const&,
        boat::db::rowset const&,
        std::stop_token) override
    {
        throw err;
    }

    boat::db::table create(boat::db::table const&) override { throw err; }

    void drop(std::string_view, std::string_view) override { throw err; }

    boat::db::raster get_raster(boat::db::layer const&) override { throw err; }

    std::generator<std::pair<boat::tile, boat::gil::any_image>> read(
        boat::db::raster,
        std::vector<boat::tile>) override
    {
        throw err;
        co_return;
    }

    void write(  //
        boat::db::raster const&,
        boat::db::rect const&,
        boat::gil::any_image_view) override
    {
        throw err;
    }

    void set_autocommit(bool) override { throw err; }

    void commit() override { throw err; }
};

}  // namespace

std::unique_ptr<boat::db::catalog> make_catalog(std::string_view address)
{
    if (address == "echo://")
        return std::make_unique<echo>();
    return boat::make_catalog(address);
}
