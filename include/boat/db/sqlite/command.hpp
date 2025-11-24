// Andrew Naplavkov

#ifndef BOAT_DB_SQLITE_COMMAND_HPP
#define BOAT_DB_SQLITE_COMMAND_HPP

#include <boat/db/command.hpp>
#include <boat/db/sqlite/detail/utility.hpp>

namespace boat::db::sqlite {

class command : public db::command {
    unique_ptr<sqlite3, sqlite3_close_v2> dbc_;
    unique_ptr<void, spatialite_cleanup_ex> spatial_;

public:
    explicit command(char const* file)
    {
        constexpr int flags =
            SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE | SQLITE_OPEN_NOMUTEX;
        sqlite3* dbc;
        check(sqlite3_open_v2(file, &dbc, flags, 0), dbc_);
        dbc_.reset(dbc);
        spatial_.reset(spatialite_alloc_connection());
        spatialite_init_ex(dbc_.get(), spatial_.get(), 0);
    }

    pfr::rowset exec(query const& qry) override
    {
        auto ret = pfr::rowset{};
        auto const sql = qry.sql(id_quote(), param_mark());
        auto first = sql.data();
        auto last = first + sql.size();
        auto params = qry.params();
        auto it = params.begin();
        while (first != last) {
            ret = {};
            sqlite3_stmt* stmt;
            check(sqlite3_prepare_v2(
                      dbc_.get(), first, int(last - first), &stmt, &first),
                  dbc_);
            auto _ = unique_ptr<sqlite3_stmt, sqlite3_finalize>{stmt};
            int num_params = sqlite3_bind_parameter_count(stmt);
            for (int i{}; i < num_params && it != params.end(); ++i, ++it)
                check(bind_value(stmt, i + 1, *it), dbc_);
            ret.columns.resize(sqlite3_column_count(stmt));
            for (int i{}; i < static_cast<int>(ret.columns.size()); ++i)
                ret.columns[i] = sqlite3_column_name(stmt, i);
            int ec = sqlite3_step(stmt);
            for (; SQLITE_DONE != ec; ec = sqlite3_step(stmt)) {
                check(ec, dbc_);
                auto& row = ret.rows.emplace_back(ret.columns.size());
                for (int i{}; i < static_cast<int>(ret.columns.size()); ++i)
                    row[i] = column_value(stmt, i);
            }
        }
        return ret;
    }

    void set_autocommit(bool on) override { exec(on ? "rollback;" : "begin;"); }
    void commit() override { exec("commit;begin;"); }
    char id_quote() override { return '"'; }
    std::string param_mark() override { return "?"; }
    std::string dbms() override { return "sqlite"; }
};

}  // namespace boat::db::sqlite

#endif  // BOAT_DB_SQLITE_COMMAND_HPP
