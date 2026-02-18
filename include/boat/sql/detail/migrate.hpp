// Andrew Naplavkov

#ifndef BOAT_SQL_MIGRATE_HPP
#define BOAT_SQL_MIGRATE_HPP

#include <boat/db/command.hpp>
#include <boat/sql/detail/dialects/dialects.hpp>

namespace boat::sql {

inline table migrate(db::command& cmd, table const& tbl)
{
    auto& dial = dialects::find(cmd.lcase_dbms());
    auto ret = table{
        .lcase_dbms{cmd.lcase_dbms()},
        .schema_name{
            tbl.schema_name.empty()
                ? pfr::get<std::string>(cmd.exec(dial.schema()).value())
                : tbl.schema_name},
        .table_name{tbl.table_name},
        .index_keys{tbl.index_keys},
    };
    for (auto& col : tbl.columns) {
        auto ptr = adaptors::make(tbl.lcase_dbms, col);
        auto [type, len, epsg] = ptr->to_type(ret.lcase_dbms);
        auto srid =
            epsg > 0 ? pfr::get<int>(cmd.exec(dial.srid(epsg)).value()) : 0;
        ret.columns.emplace_back(col.column_name, type, len, srid, epsg);
    }
    return ret;
}

}  // namespace boat::sql

#endif  // BOAT_SQL_MIGRATE_HPP
