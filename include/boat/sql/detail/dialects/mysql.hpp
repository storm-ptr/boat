// Andrew Naplavkov

#ifndef BOAT_SQL_DIALECTS_MYSQL_HPP
#define BOAT_SQL_DIALECTS_MYSQL_HPP

#include <boat/sql/detail/dialects/dialect.hpp>
#include <boat/sql/detail/syntax.hpp>

namespace boat::sql::dialects {

struct mysql : dialect {
    bool match(std::string_view dbms_name) const override
    {
        return dbms_name.contains("mysql");
    }

    db::query layers() const override
    {
        return {"\n select table_schema, table_name, column_name",
                "\n from information_schema.st_geometry_columns"};
    }

    db::query columns(std::string_view schema_name,
                      std::string_view table_name) const override
    {
        auto q = db::query{};
        q << "\n with l (table_schema, table_name, column_name, srid) as ("
          << "\n  select table_schema, table_name, column_name, srs_id"
          << "\n  from information_schema.st_geometry_columns"
          << "\n ), r (srid, epsg) as ("
          << "\n  select srs_id, organization_coordsys_id"
          << "\n  from information_schema.st_spatial_reference_systems"
          << "\n  where organization = 'EPSG'"
          << "\n )"
          << "\n select column_name"
          << "\n , data_type"
          << "\n , coalesce(character_maximum_length, datetime_precision)"
          << "\n , srid"
          << "\n , epsg"
          << "\n from information_schema.columns c"
          << "\n left join l using (table_schema, table_name, column_name)"
          << "\n left join r using (srid)"
          << "\n where c.table_schema = " << pfr::variant(schema_name)
          << "\n and c.table_name = " << pfr::variant(table_name);
        return q;
    }

    db::query index_keys(std::string_view schema_name,
                         std::string_view table_name) const override
    {
        auto q = db::query{};
        q << "\n select null index_schema"
          << "\n , index_name"
          << "\n , column_name"
          << "\n , (collation = 'D') is_descending_key"
          << "\n , (expression is not null) is_partial"
          << "\n , (index_name = 'PRIMARY') is_primary_key"
          << "\n , (not non_unique) is_unique"
          << "\n , seq_in_index ordinal"
          << "\n from information_schema.statistics"
          << "\n where table_schema = " << pfr::variant(schema_name)
          << "\n and table_name = " << pfr::variant(table_name);
        return q;
    }

    db::query select(table const& tbl, page const& req) const override
    {
        auto q = db::query{};
        q << "\n select " << select_list{tbl, req.select_list} << "\n from "
          << id{tbl} << order_by{tbl, req.order_by} << "\n limit "
          << to_chars(req.limit) << "\n offset " << to_chars(req.offset);
        return q;
    }

    db::query select(table const& tbl, overlap const& req) const override
    {
        auto col = find_or_geo(tbl.columns, req.spatial_column);
        auto q = db::query{};
        q << "\n select " << select_list{tbl, req.select_list} << "\n from "
          << id{tbl} << "\n where MBRIntersects("
          << polygon{tbl, *col, req.xmin, req.ymin, req.xmax, req.ymax} << ", "
          << db::id(col->column_name) << ")\n limit " << to_chars(req.limit);
        return q;
    }

    db::query schema() const override { return "select schema()"; }

    db::query srid(int epsg) const override
    {
        return {
            "\n select srs_id"
            "\n from information_schema.st_spatial_reference_systems",
            "\n where organization = 'EPSG'",
            "\n and organization_coordsys_id = ",
            to_chars(epsg)};
    }

    db::query create(table const& tbl) const override
    {
        auto q = db::query{};
        q << "\n create table " << id{tbl};
        auto sep = "\n ( ";
        for (auto& col : tbl.columns) {
            q << std::exchange(sep, "\n , ") << db::id{col.column_name} << " "
              << col.type_name;
            if (col.srid > 0)
                q << " not null srid " << to_chars(col.srid);
            else if (col.length > 0 && !col.type_name.contains(" "))
                q << "(" << to_chars(col.length) << ")";
        }
        return q << "\n );" << create_indices{tbl};
    }
};

}  // namespace boat::sql::dialects

#endif  // BOAT_SQL_DIALECTS_MYSQL_HPP
