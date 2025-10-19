// Andrew Naplavkov

#ifndef BOAT_SQL_API_HPP
#define BOAT_SQL_API_HPP

#include <boat/sql/detail/insert.hpp>
#include <boat/sql/detail/migrate.hpp>

namespace boat::sql {

inline std::vector<layer> get_layers(db::command& cmd)
{
    auto ret = cmd.exec(dialects::find(cmd.dbms()).layers()) |
               pfr::view<layer> | std::ranges::to<std::vector>();
    std::ranges::sort(ret, {}, [](auto& lyr) {
        return std::tuple(lyr.schema_name | unicode::string<unicode::point>,
                          lyr.table_name | unicode::string<unicode::point>,
                          lyr.column_name | unicode::string<unicode::point>);
    });
    return ret;
}

inline table get_table(db::command& cmd,
                       std::string_view schema_name,
                       std::string_view table_name)
{
    auto& dial = dialects::find(cmd.dbms());
    auto ret = table{
        .dbms_name{cmd.dbms() | unicode::lower | unicode::string<char>},
        .schema_name{schema_name},
        .table_name{table_name},
        .columns{std::from_range,
                 cmd.exec(dial.columns(schema_name, table_name)) |
                     pfr::view<column> | std::views::transform([](auto col) {
                         col.type_name = col.type_name | unicode::lower |
                                         unicode::string<char>;
                         return col;
                     })},
        .index_keys{std::from_range,
                    cmd.exec(dial.index_keys(schema_name, table_name)) |
                        pfr::view<index_key>}};
    std::ranges::sort(ret.columns, {}, [](auto& col) {
        return col.column_name | unicode::string<unicode::point>;
    });
    std::ranges::sort(ret.index_keys, {}, [](auto& key) {
        return std::tuple(!key.primary,
                          key.schema_name | unicode::string<unicode::point>,
                          key.index_name | unicode::string<unicode::point>,
                          key.ordinal);
    });
    return ret;
}

template <class T>
pfr::rowset select(db::command& cmd, table const& tbl, T const& req)
    requires requires(dialects::dialect* dial) { dial->select(tbl, req); }
{
    return cmd.exec(dialects::find(cmd.dbms()).select(tbl, req));
}

inline void insert(db::command& cmd, table const& tbl, pfr::rowset const& vals)
{
    for (auto qry : inserts(tbl, vals))
        cmd.exec(qry);
}

inline table create(db::command& cmd, table const& tbl)
{
    auto draft = migrate(cmd, tbl);
    cmd.exec(dialects::find(cmd.dbms()).create(draft));
    return get_table(cmd, draft.schema_name, draft.table_name);
}

inline std::string ddl(db::command& cmd, table const& tbl)
{
    auto qry = dialects::find(cmd.dbms()).create(migrate(cmd, tbl));
    return qry.sql(cmd.id_quote(), {});
}

}  // namespace boat::sql

#endif  // BOAT_SQL_API_HPP
