// Andrew Naplavkov

#ifndef BOAT_SQL_DIALECTS_SQLITE_HPP
#define BOAT_SQL_DIALECTS_SQLITE_HPP

#include <boat/sql/detail/dialects/dialect.hpp>
#include <boat/sql/detail/syntax.hpp>

namespace boat::sql::dialects {

struct sqlite : dialect {
    bool match(std::string_view dbms_name) const override
    {
        return dbms_name.contains("sqlite");
    }

    db::query layers() const override
    {
        return {"\n select null table_schema",
                "\n      , m.name table_name",
                "\n      , (select c.name from pragma_table_info(m.name) c",
                "\n         where f_geometry_column like c.name) column_name",
                "\n from geometry_columns, sqlite_master m",
                "\n where f_table_name like m.name"};
    }

    db::query columns(std::string_view,
                      std::string_view table_name) const override
    {
        auto q = db::query{};
        q << "\n select c.*"
          << "\n      , (select auth_srid from spatial_ref_sys r"
          << "\n         where auth_name = 'epsg' and c.srid = r.srid) epsg"
          << "\n from ("
          << "\n   select name"
          << "\n        , type"
          << "\n        , null length"
          << "\n        , (select srid from geometry_columns"
          << "\n           where f_table_name like " << pfr::variant(table_name)
          << "\n           and f_geometry_column like name) srid"
          << "\n   from pragma_table_info(" << pfr::variant(table_name) << ")"
          << "\n ) c";
        return q;
    }

    db::query index_keys(std::string_view,
                         std::string_view table_name) const override
    {
        auto q = db::query{};
        q << "\n select null index_schema"
          << "\n      , 'idx_' || f_table_name || '_' || f_geometry_column"
          << "\n        index_name"
          << "\n      , name column_name"
          << "\n      , 0 is_descending_key"
          << "\n      , 0 is_partial"
          << "\n      , 0 is_primary_key"
          << "\n      , 0 is_unique"
          << "\n      , 1 ordinal"
          << "\n from geometry_columns"
          << "\n    , pragma_table_info(" << pfr::variant(table_name) << ")"
          << "\n where f_table_name like " << pfr::variant(table_name)
          << "\n and f_geometry_column like name"
          << "\n and spatial_index_enabled"
          << "\n union"
          << "\n select null, 'primary', name, 0, 0, 1, 1, pk"
          << "\n from pragma_table_info(" << pfr::variant(table_name) << ")"
          << "\n where pk > 0"
          << "\n union"
          << "\n select null"
          << "\n      , i.name"
          << "\n      , c.name"
          << "\n      , c.desc"
          << "\n      , i.partial"
          << "\n      , 0"
          << "\n      , i." << db::id{"unique"}  //
          << "\n      , c.seqno + 1"
          << "\n from pragma_index_list(" << pfr::variant(table_name) << ") i"
          << "\n    , pragma_index_xinfo(i.name) c"
          << "\n where key and origin <> 'pk'";
        return q;
    }

    db::query select(table const& tbl, page const& req) const override
    {
        auto q = db::query{};
        q << "\n select " << select_list{tbl, req.select_list}  //
          << "\n from " << db::id{tbl.table_name} << order_by{tbl, req.order_by}
          << "\n limit " << to_chars(req.limit)  //
          << "\n offset " << to_chars(req.offset);
        return q;
    }

    db::query select(table const& tbl, box const& req) const override
    {
        auto col = find_or_geo(tbl.columns, req.spatial_column);
        auto key = std::ranges::find(
            tbl.index_keys, col->column_name, &index_key::column_name);
        auto q = db::query{};
        q << "\n select " << select_list{tbl, req.select_list}  //
          << "\n from " << db::id{tbl.table_name}
          << "\n where rowid in (select pkid from " << db::id{key->index_name}
          << "\n   where xmax >= " << pfr::variant(req.xmin)
          << "\n   and xmin <= " << pfr::variant(req.xmax)
          << "\n   and ymax >= " << pfr::variant(req.ymin)
          << "\n   and ymin <= " << pfr::variant(req.ymax)  //
          << "\n   limit " << to_chars(req.limit) << ");";
        return q;
    }

    db::query schema() const override { return "select null"; }

    db::query srid(int epsg) const override
    {
        return {"\n select srid from spatial_ref_sys",
                "\n where auth_name like 'epsg' and auth_srid = ",
                to_chars(epsg)};
    }

    db::query create(table const& tbl) const override
    {
        auto q = db::query{};
        q << "\n create table " << db::id{tbl.table_name};
        auto sep = "\n ( ";
        for (auto& col : tbl.columns | std::views::filter(std::not_fn(geo)))
            q << std::exchange(sep, "\n , ") << db::id{col.column_name} << " "
              << col.type_name;
        for (auto idx :
             tbl.indices() | std::views::filter(primary) | std::views::take(1))
            q << std::exchange(sep, "\n , ") << "primary key "
              << index_spec{idx};
        q << "\n );";
        for (auto& col : tbl.columns | std::views::filter(geo))
            q << "\n select AddGeometryColumn(" << pfr::variant{tbl.table_name}
              << ", " << pfr::variant{col.column_name} << ", "
              << to_chars(col.srid) << ", 'GEOMETRY', 'XY');";
        auto i = 0;
        for (auto idx : tbl.indices() | std::views::filter(constructible) |
                            std::views::filter(std::not_fn(primary))) {
            auto key = std::ranges::begin(idx);
            if (!any_geo(tbl.columns, key->column_name))
                q << "\n create " << (key->unique ? "unique " : "") << "index "
                  << concat("_", tbl.table_name, "_", ++i) << " on "
                  << db::id{tbl.table_name} << " " << index_spec{idx} << ";";
            else if (std::ranges::size(idx) == 1u)
                q << "\n select CreateSpatialIndex("
                  << pfr::variant{tbl.table_name} << ", "
                  << pfr::variant{key->column_name} << ");";
        }
        return q;
    }
};

}  // namespace boat::sql::dialects

#endif  // BOAT_SQL_DIALECTS_SQLITE_HPP
