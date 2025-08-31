// Andrew Naplavkov

#ifndef BOAT_SQL_MIGRATE_HPP
#define BOAT_SQL_MIGRATE_HPP

#include <boat/db/command.hpp>
#include <boat/sql/detail/adaptors/adaptors.hpp>
#include <boat/sql/detail/dialects/dialects.hpp>

namespace boat::sql {

inline table migrate(db::command& cmd, table const& tbl)
{
    auto& dial = dialects::find(cmd.dbms());
    auto ret = table{
        .dbms_name{cmd.dbms() | unicode::lower | unicode::string<char>},
        .schema_name{tbl.schema_name.empty()
                         ? get<std::string>(cmd.exec(dial.schema()).value())
                         : tbl.schema_name},
        .table_name{tbl.table_name},
        .index_keys{tbl.index_keys}};
    for (auto& col : tbl.columns) {
        auto ptr = adaptors::try_create(tbl, col);
        if (!ptr)
            continue;
        auto [type, len, epsg] = ptr->migrate(ret.dbms_name);
        auto srid = epsg > 0 ? get<int>(cmd.exec(dial.srid(epsg)).value()) : 0;
        ret.columns.emplace_back(col.column_name, type, len, srid, epsg);
    }
    for (auto& key : ret.index_keys)
        if (!std::ranges::contains(
                ret.columns, key.column_name, &column::column_name))
            key.column_name.clear();
    return ret;
}

}  // namespace boat::sql

#endif  // BOAT_SQL_MIGRATE_HPP
