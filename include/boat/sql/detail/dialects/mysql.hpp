// Andrew Naplavkov

#ifndef BOAT_SQL_DIALECTS_MYSQL_HPP
#define BOAT_SQL_DIALECTS_MYSQL_HPP

#include <boat/sql/detail/dialects/dialect.hpp>
#include <boat/sql/detail/manip.hpp>

namespace boat::sql::dialects {

struct mysql : dialect {
    db::query vectors() const override
    {
        return "\n select table_schema, table_name, column_name"
               "\n from information_schema.st_geometry_columns";
    }

    db::query columns(std::string_view schema_name,
                      std::string_view table_name) const override
    {
        return {
            "\n with l as ("
            "\n  select table_schema, table_name, column_name, srs_id srid"
            "\n  from information_schema.st_geometry_columns"
            "\n ), r as ("
            "\n  select srs_id srid, organization_coordsys_id epsg"
            "\n  from information_schema.st_spatial_reference_systems"
            "\n  where organization = 'EPSG'"
            "\n )"
            "\n select null, column_name, data_type"
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
            "\n select null , index_name, column_name, (collation = 'D')"
            "\n , (expression is not null), (index_name = 'PRIMARY')"
            "\n , (not non_unique), seq_in_index"
            "\n from information_schema.statistics"
            "\n where table_schema = ",
            db::variant(schema_name),
            "\n and table_name = ",
            db::variant(table_name)};
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
          << id{tbl} << "\n where MBRIntersects("
          << rect{tbl.dbms, col, rq.xmin, rq.ymin, rq.xmax, rq.ymax} << ", "
          << db::id(col.column_name) << ")\n limit " << to_chars(rq.limit);
        return q;
    }

    db::query schema() const override { return "select schema()"; }

    db::query srid(int epsg) const override
    {
        return {
            "\n select srs_id"
            "\n from information_schema.st_spatial_reference_systems"
            "\n where organization = 'EPSG' and organization_coordsys_id = ",
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
                q << " not null srid " << to_chars(col.srid);
            else if (col.length > 0)
                q << "(" << to_chars(col.length) << ")";
        }
        return q << "\n );" << create_indices{tbl};
    }
};

}  // namespace boat::sql::dialects

#endif  // BOAT_SQL_DIALECTS_MYSQL_HPP
