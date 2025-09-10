// Andrew Naplavkov

#ifndef BOAT_DB_LIBMYSQL_FETCH_HPP
#define BOAT_DB_LIBMYSQL_FETCH_HPP

#include <boat/db/libmysql/detail/utility.hpp>
#include <boat/pfr/rowset.hpp>

namespace boat::db::libmysql {

inline pfr::rowset fetch(MYSQL* dbc)
{
    auto ret = pfr::rowset{};
    auto res =
        unique_ptr<MYSQL_RES, mysql_free_result>{mysql_store_result(dbc)};
    if (!res)
        return ret;
    ret.columns.resize(mysql_num_fields(res.get()));
    auto fields = mysql_fetch_fields(res.get());
    for (int col = 0; col < ret.columns.size(); ++col)
        ret.columns[col] = fields[col].name;
    ret.rows.resize(mysql_num_rows(res.get()));
    for (int row = 0; row < ret.rows.size(); ++row) {
        auto data = mysql_fetch_row(res.get());
        auto lengths = mysql_fetch_lengths(res.get());
        ret.rows[row].resize(ret.columns.size());
        for (int col{}; col < ret.columns.size(); ++col)
            ret.rows[row][col] =
                get_value(fields[col], data[col], lengths[col]);
    }
    return ret;
}

inline pfr::rowset fetch(MYSQL_STMT* stmt)
{
    auto ret = pfr::rowset{};
    auto update_len = true;
    check(!mysql_stmt_attr_set(stmt, STMT_ATTR_UPDATE_MAX_LENGTH, &update_len),
          stmt);
    check(!mysql_stmt_store_result(stmt), stmt);
    auto _ = unique_ptr<MYSQL_STMT, mysql_stmt_free_result>{stmt};
    auto res = unique_ptr<MYSQL_RES, mysql_free_result>{
        mysql_stmt_result_metadata(stmt)};
    if (!res)
        return ret;
    ret.columns.resize(mysql_num_fields(res.get()));
    auto binds = std::vector<MYSQL_BIND>(ret.columns.size());
    auto bufs = std::vector<buffer>(ret.columns.size());
    auto fields = mysql_fetch_fields(res.get());
    for (int col = 0; col < ret.columns.size(); ++col) {
        ret.columns[col] = fields[col].name;
        bufs[col].str.resize(fields[col].max_length);
        binds[col].buffer = bufs[col].str.data();
        binds[col].buffer_length = fields[col].max_length;
        binds[col].buffer_type = MYSQL_TYPE_STRING;
        binds[col].is_null = &bufs[col].null;
        binds[col].length = &bufs[col].len;
    }
    check(!mysql_stmt_bind_result(stmt, binds.data()), stmt);
    ret.rows.resize(mysql_stmt_num_rows(stmt));
    for (int row = 0; row < ret.rows.size(); ++row) {
        check(!mysql_stmt_fetch(stmt), stmt);
        ret.rows[row].resize(ret.columns.size());
        for (int col{}; col < ret.columns.size(); ++col)
            if (!bufs[col].null)
                ret.rows[row][col] =
                    get_value(fields[col], bufs[col].str.data(), bufs[col].len);
    }
    return ret;
}

}  // namespace boat::db::libmysql

#endif  // BOAT_DB_LIBMYSQL_FETCH_HPP
