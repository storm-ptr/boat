// Andrew Naplavkov

#ifndef BOAT_SQL_API_HPP
#define BOAT_SQL_API_HPP

#include <boat/sql/detail/insert.hpp>
#include <boat/sql/detail/migrate.hpp>

namespace boat::sql {

inline std::vector<layer> get_layers(db::command& cmd)
{
    auto ret = cmd.exec(dialects::find(cmd.lcase_dbms()).layers()) |
               pfr::view<layer> | std::ranges::to<std::vector>();
    std::ranges::sort(ret, {}, [](auto& lyr) {
        return std::tuple{
            lyr.schema_name | unicode::utf32,
            lyr.table_name | unicode::utf32,
            lyr.column_name | unicode::utf32,
        };
    });
    return ret;
}

inline table describe(  //
    db::command& cmd,
    std::string_view schema_name,
    std::string_view table_name)
{
    auto& dial = dialects::find(cmd.lcase_dbms());
    auto ret = table{
        .lcase_dbms{cmd.lcase_dbms()},
        .schema_name{schema_name},
        .table_name{table_name},
        .columns{
            std::from_range,
            cmd.exec(dial.columns(schema_name, table_name)) |
                pfr::view<column> | std::views::transform([](auto col) {
                    col.lcase_type = unicode::to_lower(col.lcase_type);
                    return col;
                }),
        },
        .index_keys{
            std::from_range,
            cmd.exec(dial.index_keys(schema_name, table_name)) |
                pfr::view<index_key>,
        },
    };
    std::ranges::sort(ret.columns, {}, [](auto& col) {
        return col.column_name | unicode::utf32;
    });
    std::ranges::sort(ret.index_keys, {}, [](auto& key) {
        return std::tuple{
            !key.primary,
            key.schema_name | unicode::utf32,
            key.index_name | unicode::utf32,
            key.ordinal,
        };
    });
    return ret;
}

inline table describe(db::command& cmd, std::string_view table_name)
{
    auto scm = cmd.exec(dialects::find(cmd.lcase_dbms()).schema()).value();
    return describe(cmd, get<std::string>(scm), table_name);
}

pfr::rowset select(db::command& cmd, table const& tbl, auto const& request)
    requires requires(dialects::dialect* dial) { dial->select(tbl, request); }
{
    return cmd.exec(dialects::find(cmd.lcase_dbms()).select(tbl, request));
}

inline void insert(db::command& cmd, table const& tbl, pfr::rowset const& vals)
{
    for (auto qry : inserts(tbl, vals))
        cmd.exec(qry);
}

inline table create(db::command& cmd, table const& tbl)
{
    auto draft = migrate(cmd, tbl);
    cmd.exec(dialects::find(cmd.lcase_dbms()).create(draft));
    return describe(cmd, draft.schema_name, draft.table_name);
}

inline std::string ddl(db::command& cmd, table const& tbl)
{
    auto qry = dialects::find(cmd.lcase_dbms()).create(migrate(cmd, tbl));
    return qry.sql(cmd.id_quote(), {});
}

}  // namespace boat::sql

#endif  // BOAT_SQL_API_HPP
