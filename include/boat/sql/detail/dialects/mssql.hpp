// Andrew Naplavkov

#ifndef BOAT_SQL_DIALECTS_MSSQL_HPP
#define BOAT_SQL_DIALECTS_MSSQL_HPP

#include <boat/sql/detail/dialects/dialect.hpp>
#include <boat/sql/detail/syntax.hpp>

namespace boat::sql::dialects {

struct mssql : dialect {
    bool match(std::string_view dbms_name) const override
    {
        return dbms_name.contains("microsoft sql server");
    }

    db::query layers() const override
    {
        return "\n select table_schema, table_name, column_name"
               "\n from information_schema.columns"
               "\n where data_type in ('geometry','geography')";
    }

    db::query columns(std::string_view schema_name,
                      std::string_view table_name) const override
    {
        return {
            "\n declare @scm nvarchar(128), @tbl nvarchar(128)"
            "\n  , @col nvarchar(128), @typ nvarchar(128), @qry nvarchar(max)"
            "\n set @scm = ",
            pfr::variant(schema_name),
            "\n set @tbl = ",
            pfr::variant(table_name),
            "\n set @qry"
            "\n  = ' select column_name, data_type'"
            "\n  + ' , coalesce(character_maximum_length, datetime_precision)'"
            "\n  + ' , 0, 0'"
            "\n  + ' from information_schema.columns'"
            "\n  + ' where table_schema = ' + quotename(@scm,'''')"
            "\n  + ' and table_name = ' + quotename(@tbl,'''')"
            "\n  + ' and data_type not in (''geography'', ''geometry'')'"
            "\n  + ' union'"
            "\n  + ' select'"
            "\n  + '  name, type, -1, srid, authorized_spatial_reference_id'"
            "\n  + ' from (values (null, null, 0)'"
            "\n declare cur cursor for"
            "\n  select column_name, data_type"
            "\n  from information_schema.columns"
            "\n  where table_schema = @scm"
            "\n  and table_name = @tbl"
            "\n  and data_type in ('geography', 'geometry')"
            "\n open cur"
            "\n fetch next from cur into @col, @typ"
            "\n while @@FETCH_STATUS = 0 begin"
            "\n  set @qry"
            "\n   +=', (' + quotename(@col,'''')"
            "\n   + '  ,' + quotename(@typ,'''')"
            "\n   + '  , coalesce(('"
            "\n   + '     select top 1 ' + quotename(@col) + '.STSrid'"
            "\n   + '     from ' + quotename(@scm) + '.' + quotename(@tbl)"
            "\n   + '    ), ('"
            "\n   + '     select cast(value as int)'"
            "\n   + '     from fn_listextendedproperty(''srid'''"
            "\n   + '     , ''schema'', ' + quotename(@scm,'''')"
            "\n   + '     , ''table'', ' + quotename(@tbl,'''')"
            "\n   + '     , ''column'', ' + quotename(@col,'''') + ')'"
            "\n   + '    ), 0))'"
            "\n  fetch next from cur into @col, @typ"
            "\n end"
            "\n close cur deallocate cur"
            "\n set @qry"
            "\n  +=' ) as l(name, type, srid)'"
            "\n  + ' left join sys.spatial_reference_systems'"
            "\n  + ' on srid = spatial_reference_id'"
            "\n  + ' where name is not null and authority_name = ''epsg'''"
            "\n exec(@qry)"};
    }

    db::query index_keys(std::string_view schema_name,
                         std::string_view table_name) const override
    {
        return {
            "\n select null index_schema"
            "\n , name index_name"
            "\n , col_name(c.object_id, column_id) column_name"
            "\n , is_descending_key"
            "\n , has_filter is_partial"
            "\n , is_primary_key"
            "\n , is_unique"
            "\n , case key_ordinal when 0"
            "\n   then 1 else key_ordinal end ordinal"
            "\n from sys.indexes i, sys.index_columns c"
            "\n where i.index_id = c.index_id"
            "\n and i.object_id = c.object_id"
            "\n and i.object_id = object_id(",
            pfr::variant(concat(schema_name, '.', table_name)),
            ")"};
    }

    db::query select(table const& tbl, page const& req) const override
    {
        auto q = db::query{};
        q << "\n select ";
        if (!req.offset)
            q << "top(" << to_chars(req.limit) << ") ";
        q << select_list{tbl, req.select_list} << "\n from " << id{tbl}
          << order_by{tbl, req.order_by};
        if (req.offset)
            q << "\n offset " << to_chars(req.offset) << " rows"
              << "\n fetch next " << to_chars(req.limit) << " rows only";
        return q;
    }

    db::query select(table const& tbl, overlap const& req) const override
    {
        auto col = find_or_geo(tbl.columns, req.spatial_column);
        auto q = db::query{};
        q << "\n declare @g " << col->type_name << "\n set @g = "
          << polygon{tbl, *col, req.xmin, req.ymin, req.xmax, req.ymax}
          << "\n select top(" << to_chars(req.limit) << ") "
          << select_list{tbl, req.select_list} << "\n from " << id{tbl}
          << "\n where " << db::id{col->column_name} << ".Filter(@g) = 1";
        return q;
    }

    db::query schema() const override { return "select schema_name()"; }

    db::query srid(int epsg) const override
    {
        return {
            "\n select spatial_reference_id"
            "\n from sys.spatial_reference_systems"
            "\n where authority_name = 'epsg'"
            "\n and authorized_spatial_reference_id = ",
            to_chars(epsg)};
    }

    db::query create(table const& tbl) const override
    {
        auto q = db::query{};
        q << "\n create table " << id{tbl};
        for (auto sep = "\n ( "; auto& col : tbl.columns) {
            q << std::exchange(sep, "\n , ") << db::id{col.column_name} << " "
              << col.type_name;
            if (col.length > 0 && !col.type_name.contains(" "))
                q << "(" << to_chars(col.length) << ")";
            else if (col.length < 0 && any({"nvarchar", "varbinary", "varchar"},
                                           equal(col.type_name)))
                q << "(max)";
            if (has_primary(tbl.index_keys, col.column_name))
                q << " not null";
        }
        q << "\n );";
        for (auto& col : tbl.columns | std::views::filter(geo))
            q << "\n exec sp_addextendedproperty @name = 'srid'"
              << "\n , @value = " << to_chars(col.srid)
              << "\n , @level0type = 'schema'"
              << "\n , @level0name = " << pfr::variant(tbl.schema_name)
              << "\n , @level1type = 'table'"
              << "\n , @level1name = " << pfr::variant(tbl.table_name)
              << "\n , @level2type = 'column'"
              << "\n , @level2name = " << pfr::variant(col.column_name);
        return q << "\n ;" << create_indices{tbl};
    }
};

}  // namespace boat::sql::dialects

#endif  // BOAT_SQL_DIALECTS_MSSQL_HPP
