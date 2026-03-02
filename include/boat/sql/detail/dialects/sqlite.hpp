// Andrew Naplavkov

#ifndef BOAT_SQL_DIALECTS_SQLITE_HPP
#define BOAT_SQL_DIALECTS_SQLITE_HPP

#include <boat/sql/detail/dialects/dialect.hpp>
#include <boat/sql/detail/manip.hpp>

namespace boat::sql::dialects {

struct sqlite : dialect {
    db::query layers() const override
    {
        return "\n select null, m.name"
               "\n , (select c.name from pragma_table_info(m.name) c"
               "\n    where f_geometry_column like c.name)"
               "\n from geometry_columns, sqlite_master m"
               "\n where f_table_name like m.name";
    }

    db::query columns(std::string_view,
                      std::string_view table_name) const override
    {
        return {
            "\n select c.*"
            "\n , (select auth_srid from spatial_ref_sys r"
            "\n    where auth_name = 'epsg' and c.srid = r.srid) epsg"
            "\n from ("
            "\n  select null, name, type, 0"
            "\n  , (select srid from geometry_columns"
            "\n     where f_table_name like ",
            db::variant(table_name),
            "\n     and f_geometry_column like name) srid"
            "\n  from pragma_table_info(",
            db::variant(table_name),
            ")) c"};
    }

    db::query index_keys(std::string_view,
                         std::string_view table_name) const override
    {
        auto q = db::query{};
        q << "\n select null"
          << "\n , 'idx_' || f_table_name || '_' || f_geometry_column"
          << "\n , name, 0, 0, 0, 0, 1"
          << "\n from geometry_columns"
          << "\n , pragma_table_info(" << db::variant(table_name) << ")"
          << "\n where f_table_name like " << db::variant(table_name)
          << "\n and f_geometry_column like name"
          << "\n and spatial_index_enabled"
          << "\n union"
          << "\n select null, 'primary', name, 0, 0, 1, 1, pk"
          << "\n from pragma_table_info(" << db::variant(table_name) << ")"
          << "\n where pk > 0"
          << "\n union"
          << "\n select null, i.name, c.name, c.desc, i.partial, 0"
          << "\n , i." << db::id{"unique"} << ", c.seqno + 1"
          << "\n from pragma_index_list(" << db::variant(table_name) << ") i"
          << "\n , pragma_index_xinfo(i.name) c"
          << "\n where key and origin <> 'pk'";
        return q;
    }

    db::query select(db::table const& tbl, db::page const& rq) const override
    {
        auto q = db::query{};
        q << "\n select " << select_list{tbl, rq.select_list}  //
          << "\n from " << db::id{tbl.table_name} << order_by{tbl, rq.order_by}
          << "\n limit " << to_chars(rq.limit)  //
          << "\n offset " << to_chars(rq.offset);
        return q;
    }

    db::query select(db::table const& tbl, db::bbox const& rq) const override
    {
        auto& col = find_geo(tbl.columns, rq.layer_column);
        auto key = std::ranges::find(
            tbl.index_keys, col.column_name, &db::index_key::column_name);
        check(key != tbl.index_keys.end(), "no spatial index");
        auto q = db::query{};
        q << "\n select " << select_list{tbl, rq.select_list}  //
          << "\n from " << db::id{tbl.table_name}
          << "\n where rowid in (select pkid from " << db::id{key->index_name}
          << "\n  where xmax >= " << db::variant(rq.xmin)
          << "\n  and xmin <= " << db::variant(rq.xmax)
          << "\n  and ymax >= " << db::variant(rq.ymin)
          << "\n  and ymin <= " << db::variant(rq.ymax)  //
          << "\n  limit " << to_chars(rq.limit) << ");";
        return q;
    }

    db::query schema() const override { return "select null"; }

    db::query srid(int epsg) const override
    {
        return {
            "\n select srid from spatial_ref_sys"
            "\n where auth_name like 'epsg' and auth_srid = ",
            to_chars(epsg)};
    }

    db::query create(db::table const& tbl) const override
    {
        auto q = db::query{};
        q << "\n create table " << db::id{tbl.table_name};
        auto sep{"\n ( "};
        for (auto& col : tbl.columns | std::views::filter(std::not_fn(geo)))
            q << std::exchange(sep, "\n , ") << db::id{col.column_name} << " "
              << col.type_name;
        for (auto idx :
             tbl.indices() | std::views::filter(primary) | std::views::take(1))
            q << std::exchange(sep, "\n , ") << "primary key "
              << index_spec{idx};
        q << "\n );";
        for (auto& col : tbl.columns | std::views::filter(geo))
            q << "\n select AddGeometryColumn(" << db::variant{tbl.table_name}
              << ", " << db::variant{col.column_name} << ", "
              << to_chars(col.srid) << ", 'GEOMETRY', 'XY');";
        auto i = 0;
        for (auto idx : tbl.indices() | std::views::filter(constructible) |
                            std::views::filter(std::not_fn(primary))) {
            auto key = std::ranges::begin(idx);
            if (!geo(tbl.columns, key->column_name))
                q << "\n create " << (key->unique ? "unique " : "") << "index "
                  << concat("_", tbl.table_name, "_", ++i) << " on "
                  << db::id{tbl.table_name} << " " << index_spec{idx} << ";";
            else if (std::ranges::size(idx) == 1u)
                q << "\n select CreateSpatialIndex("
                  << db::variant{tbl.table_name} << ", "
                  << db::variant{key->column_name} << ");";
        }
        return q;
    }
};

}  // namespace boat::sql::dialects

#endif  // BOAT_SQL_DIALECTS_SQLITE_HPP
