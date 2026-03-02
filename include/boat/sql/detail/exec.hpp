// Andrew Naplavkov

#ifndef BOAT_SQL_EXEC_HPP
#define BOAT_SQL_EXEC_HPP

#include <boat/db/command.hpp>
#include <boat/sql/detail/dialects/dialects.hpp>

namespace boat::sql {

inline std::string current_schema_or(db::command& cmd, std::string_view schema)
{
    auto ret = std::string{schema};
    if (ret.empty())
        ret = db::get<std::string>(
            cmd.exec(dialects::find(cmd.dbms()).schema()).value());
    return ret;
}

inline db::table migrate(db::command& cmd, db::table const& tbl)
{
    auto ret = db::table{
        .dbms{cmd.dbms()},
        .schema_name{current_schema_or(cmd, tbl.schema_name)},
        .table_name{tbl.table_name},
        .index_keys{tbl.index_keys},
    };
    for (auto& in : tbl.columns) {
        auto out = adaptors::make(tbl.dbms, in)->migrate(ret.dbms);
        out.srid = out.epsg
                       ? db::get<int>(
                             cmd.exec(dialects::find(cmd.dbms()).srid(out.epsg))
                                 .value())
                       : 0;
        ret.columns.push_back(std::move(out));
    }
    return ret;
}

}  // namespace boat::sql

#endif  // BOAT_SQL_EXEC_HPP
