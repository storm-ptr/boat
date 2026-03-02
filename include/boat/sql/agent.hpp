// Andrew Naplavkov

#ifndef BOAT_SQL_AGENT_HPP
#define BOAT_SQL_AGENT_HPP

#include <boat/db/agent.hpp>
#include <boat/sql/detail/exec.hpp>

namespace boat::sql {

struct agent : db::agent {
    std::unique_ptr<db::command> command;

    std::vector<db::layer> get_layers() override
    {
        auto ret = command->exec(dialects::find(command->dbms()).layers()) |
                   db::view<db::layer> | std::ranges::to<std::vector>();
        std::ranges::sort(ret, {}, [](auto& lyr) {
            return std::tuple{lyr.schema_name | unicode::utf32,
                              lyr.table_name | unicode::utf32,
                              lyr.column_name | unicode::utf32};
        });
        return ret;
    }

    db::table describe(std::string_view schema_name,
                       std::string_view table_name) override
    {
        auto& dial = dialects::find(command->dbms());
        auto scm = current_schema_or(*command, schema_name);
        auto ret = db::table{
            .dbms{command->dbms()},
            .schema_name{scm},
            .table_name{table_name},
            .columns{
                std::from_range,
                command->exec(dial.columns(scm, table_name)) |
                    db::view<db::column> |
                    std::views::transform(adaptors::parse(command->dbms()))},
            .index_keys{std::from_range,
                        command->exec(dial.index_keys(scm, table_name)) |
                            db::view<db::index_key>}};
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

    db::rowset select(db::table const& tbl, db::page const& rq) override
    {
        return command->exec(dialects::find(command->dbms()).select(tbl, rq));
    }

    db::rowset select(db::table const& tbl, db::bbox const& rq) override
    {
        return command->exec(dialects::find(command->dbms()).select(tbl, rq));
    }

    void insert(db::table const& tbl, db::rowset const& vals) override
    {
        auto cols = std::vector<std::unique_ptr<adaptors::adaptor>>{};
        for (auto& col : vals.columns)
            cols.push_back(adaptors::make(tbl.dbms, find(tbl.columns, col)));
        auto num_rows = std::max<size_t>(1, 999 / cols.size());
        for (auto const& rows : vals | std::views::chunk(num_rows)) {
            auto q = db::query{};
            q << "\n insert into " << id{tbl};
            for (auto sep{"\n   ("}; auto& col : vals.columns)
                q << std::exchange(sep, ", ") << db::id(col);
            q << ")\n values";
            for (auto sep1{"\n   "}; auto& row : rows) {
                q << std::exchange(sep1, "\n , ");
                for (auto sep2{"("}; auto [c, v] : std::views::zip(cols, row)) {
                    q << std::exchange(sep2, ", ");
                    c->insert(q, v);
                }
                q << ")";
            }
            command->exec(q);
        }
    }

    db::table create(db::table const& tbl) override
    {
        auto draft = migrate(*command, tbl);
        command->exec(dialects::find(command->dbms()).create(draft));
        return describe(draft.schema_name, draft.table_name);
    }

    void drop(std::string_view schema_name,
              std::string_view table_name) override
    {
        command->exec(
            {"drop table if exists ",
             id{current_schema_or(*command, schema_name), table_name}});
    }
};

}  // namespace boat::sql

#endif  // BOAT_SQL_AGENT_HPP
