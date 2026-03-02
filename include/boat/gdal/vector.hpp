// Andrew Naplavkov

#ifndef BOAT_GDAL_VECTOR_HPP
#define BOAT_GDAL_VECTOR_HPP

#include <boat/db/agent.hpp>
#include <boat/db/query.hpp>
#include <boat/gdal/dataset.hpp>
#include <boat/gdal/detail/rowset.hpp>
#include <boat/gdal/detail/table.hpp>

namespace boat::gdal {

struct agent : db::agent {
    dataset_ptr dataset;

    std::vector<db::layer> get_layers() override
    {
        auto ret = std::vector<db::layer>{};
        for (int i{}, n = GDALDatasetGetLayerCount(dataset.get()); i < n; ++i) {
            auto fd = OGR_L_GetLayerDefn(GDALDatasetGetLayer(dataset.get(), i));
            for (int j{}, m = OGR_FD_GetGeomFieldCount(fd); j < m; ++j)
                ret.push_back({.table_name = OGR_FD_GetName(fd),
                               .column_name = OGR_GFld_GetNameRef(
                                   OGR_FD_GetGeomFieldDefn(fd, j))});
        }
        std::ranges::sort(ret, {}, [](auto& lyr) {
            return std::tuple{lyr.table_name | unicode::utf32,
                              lyr.column_name | unicode::utf32};
        });
        return ret;
    }

    db::table describe(std::string_view, std::string_view table_name) override
    {
        auto tbl = std::string{table_name};
        return get_table(GDALDatasetGetLayerByName(dataset.get(), tbl.data()));
    }

    db::rowset select(db::table const& tbl, db::page const& rq) override
    {
        auto q = db::query{};
        q << "\n select * from " << db::id{tbl.table_name};
        for (auto sep{"\n order by "}; auto& key : rq.order_by)
            q << std::exchange(sep, ", ") << db::id{key.column_name}
              << (key.descending ? " desc" : "");
        q << "\n limit " << to_chars(rq.limit) << "\n offset "
          << to_chars(rq.offset);
        auto lyr = execute(dataset.get(), q.text('"', {}).data(), 0);
        auto fd = OGR_L_GetLayerDefn(lyr.get());
        return read(lyr.get(), fields::make(fd, rq.select_list), rq.limit);
    }

    db::rowset select(db::table const& tbl, db::bbox const& rq) override
    {
        auto lyr =
            GDALDatasetGetLayerByName(dataset.get(), tbl.table_name.data());
        auto fd = OGR_L_GetLayerDefn(lyr);
        OGR_L_SetSpatialFilterRectEx(
            lyr,
            std::max<>(0, OGR_FD_GetGeomFieldIndex(fd, rq.layer_column.data())),
            rq.xmin,
            rq.ymin,
            rq.xmax,
            rq.ymax);
        return read(lyr, fields::make(fd, rq.select_list), rq.limit);
    }

    void insert(db::table const& tbl, db::rowset const& vals) override
    {
        write(GDALDatasetGetLayerByName(dataset.get(), tbl.table_name.data()),
              vals);
    }

    db::table create(db::table const& tbl) override
    {
        return get_table(add_table(dataset.get(), tbl));
    }

    void drop(std::string_view, std::string_view table_name) override
    {
        delete_table(dataset.get(), table_name);
    }
};

}  // namespace boat::gdal

#endif  // BOAT_GDAL_VECTOR_HPP
