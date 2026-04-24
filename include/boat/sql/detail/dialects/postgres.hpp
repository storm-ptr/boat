// Andrew Naplavkov

#ifndef BOAT_SQL_DIALECTS_POSTGRES_HPP
#define BOAT_SQL_DIALECTS_POSTGRES_HPP

#include <boat/sql/detail/dialects/dialect.hpp>
#include <boat/sql/detail/manip.hpp>

namespace boat::sql::dialects {

struct postgres : dialect {
    db::query layers() const override
    {
        return "\n select f_table_schema, f_table_name, f_geography_column, 0"
               "\n from public.geography_columns"
               "\n union"
               "\n select f_table_schema, f_table_name, f_geometry_column, 0"
               "\n from public.geometry_columns";
    }

    db::query columns(std::string_view schema_name,
                      std::string_view table_name) const override
    {
        return {
            "\n with l (table_schema, table_name, column_name, srid) as ("
            "\n  select f_table_schema, f_table_name, f_geography_column, srid"
            "\n  from public.geography_columns"
            "\n  union"
            "\n  select f_table_schema, f_table_name, f_geometry_column, srid"
            "\n  from public.geometry_columns"
            "\n ), r as ("
            "\n  select srid, auth_srid epsg"
            "\n  from public.spatial_ref_sys where auth_name = 'EPSG'"
            "\n )"
            "\n select null, column_name"
            "\n , case data_type when 'USER-DEFINED'"
            "\n   then udt_name else data_type end"
            "\n , coalesce(character_maximum_length, datetime_precision)"
            "\n , srid, epsg"
            "\n from information_schema.columns c"
            "\n left join l using (table_schema, table_name, column_name)"
            "\n left join r using (srid)"
            "\n where c.table_schema = ",
            db::variant(schema_name),
            "\n and c.table_name = ",
            db::variant(table_name)};
    }

    db::query index_keys(std::string_view schema_name,
                         std::string_view table_name) const override
    {
        return {
            "\n with indices as ("
            "\n  select s.nspname scm"
            "\n  , t.oid tbl"
            "\n  , o.relname nm"
            "\n  , pg_get_expr(i.indpred, i.indrelid) fltr"
            "\n  , i.indisprimary prim"
            "\n  , i.indisunique  uniq"
            "\n  , i.indkey cols"
            "\n  , i.indoption opts"
            "\n  , array_lower(i.indkey, 1) lb"
            "\n  , array_upper(i.indkey, 1) ub"
            "\n  from pg_index i, pg_class o, pg_class t, pg_namespace s"
            "\n  where i.indexrelid = o.oid"
            "\n  and i.indrelid = t.oid"
            "\n  and t.relnamespace = s.oid"
            "\n  and s.nspname = ",
            db::variant(schema_name),
            "\n  and t.relname = ",
            db::variant(table_name),
            "\n ), columns as ("
            "\n  select *, generate_series(lb, ub) col from indices"
            "\n )"
            "\n select scm, nm, attname, opts[col] & 1, fltr is not null"
            "\n , prim, uniq, col + 1"
            "\n from columns left join pg_attribute"
            "\n on attrelid = tbl and attnum = cols[col]"};
    }

    db::query select(db::table const& tbl, db::page const& rq) const override
    {
        auto q = db::query{};
        q << "\n select " << select_list{tbl, rq.select_list} << "\n from "
          << id{tbl} << order_by{tbl, rq.order_by} << "\n limit "
          << to_chars(rq.limit) << "\n offset " << to_chars(rq.offset);
        return q;
    }

    db::query select(db::table const& tbl, db::bbox const& rq) const override
    {
        auto& col = find_geo(tbl.columns, rq.layer_column);
        auto q = db::query{};
        q << "\n select " << select_list{tbl, rq.select_list} << "\n from "
          << id{tbl} << "\n where " << db::id(col.column_name) << " && "
          << rect{tbl.dbms, col, rq.xmin, rq.ymin, rq.xmax, rq.ymax}
          << "\n limit " << to_chars(rq.limit);
        return q;
    }

    db::query schema() const override { return "select current_schema()"; }

    db::query srid(int epsg) const override
    {
        return {
            "\n select srid from public.spatial_ref_sys"
            "\n where lower(auth_name) = 'epsg' and auth_srid = ",
            to_chars(epsg)};
    }

    db::query create(db::table const& tbl) const override
    {
        auto q = db::query{};
        q << "\n create table " << id{tbl};
        for (auto sep{"\n ( "}; auto& col : tbl.columns) {
            q << std::exchange(sep, "\n , ") << db::id{col.column_name} << " "
              << col.type_name;
            if (geo(col))
                q << "(geometry, " << to_chars(col.srid) << ")";
            else if (col.length > 0 && !col.type_name.contains(" "))
                q << "(" << to_chars(col.length) << ")";
        }
        q << "\n );";
        for (auto idx : tbl.indices() | std::views::filter(constructible)) {
            auto key = std::ranges::begin(idx);
            auto spatial = geo(tbl.columns, key->column_name);
            if (key->primary)
                q << "\n alter table " << id{tbl} << " add primary key ";
            else
                q << "\n create " << (key->unique ? "unique " : "")
                  << "index on " << id{tbl} << (spatial ? " using gist " : " ");
            q << index_spec{idx} << ";";
        }
        return q;
    }
};

}  // namespace boat::sql::dialects

#endif  // BOAT_SQL_DIALECTS_POSTGRES_HPP
