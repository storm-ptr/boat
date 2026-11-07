// Andrew Naplavkov

#ifndef BOAT_SQL_DIALECTS_MYSQL_HPP
#define BOAT_SQL_DIALECTS_MYSQL_HPP

#include <boat/sql/detail/dialects/dialect.hpp>
#include <boat/sql/detail/manip.hpp>

namespace boat::sql::dialects {

struct mysql : dialect {
    db::query layers() const override
    {
        return "\n select table_schema, table_name, column_name, 0"
               "\n from information_schema.st_geometry_columns";
    }

    db::query columns(std::string_view schema_name, std::string_view table_name)
        const override
    {
        return {
            "\n select null, column_name, data_type"
            "\n , coalesce(character_maximum_length, datetime_precision)"
            "\n , c.srs_id"
            "\n , case r.organization when 'EPSG'"
            "\n   then r.organization_coordsys_id else null end"
            "\n , r.definition, null"
            "\n from information_schema.columns c"
            "\n left join information_schema.st_spatial_reference_systems r"
            "\n on r.srs_id = c.srs_id"
            "\n where c.table_schema = ",
            db::variant(schema_name),
            "\n and c.table_name = ",
            db::variant(table_name)};
    }

    db::query index_keys(
        std::string_view schema_name,
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
        q << "\n );";
        int i{};
        for (auto idx : tbl.indices() | std::views::filter(constructible)) {
            auto key = std::ranges::begin(idx);
            auto spatial = geo(tbl.columns, key->column_name);
            if (spatial && std::ranges::size(idx) > 1u)
                continue;
            auto type = spatial ? "spatial" : key->unique ? "unique" : "";
            if (key->primary)
                q << "\n alter table " << id{tbl} << " add primary key ";
            else
                q << "\n create " << type << " index _" << to_chars(++i)
                  << " on " << id{tbl} << " ";
            q << index_spec{idx} << ";";
        }
        return q;
    }
};

}  // namespace boat::sql::dialects

#endif  // BOAT_SQL_DIALECTS_MYSQL_HPP
