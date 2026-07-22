// Andrew Naplavkov

#ifndef BOAT_SQL_SQLITE_COMMAND_HPP
#define BOAT_SQL_SQLITE_COMMAND_HPP

#include <boat/db/command.hpp>
#include <boat/sql/sqlite/detail/utility.hpp>
#include <filesystem>

namespace boat::sql::sqlite {

class command : public db::command {
    unique_ptr<sqlite3, sqlite3_close_v2> dbc_;
    unique_ptr<void, spatialite_cleanup_ex> spatial_;

public:
    explicit command(char const* file)
    {
        constexpr int flags =
            SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE | SQLITE_OPEN_NOMUTEX;
        auto exists = std::filesystem::exists(file);
        sqlite3* dbc;
        check(sqlite3_open_v2(file, &dbc, flags, 0), dbc_);
        dbc_.reset(dbc);
        spatial_.reset(spatialite_alloc_connection());
        spatialite_init_ex(dbc_.get(), spatial_.get(), 0);
        if (!exists)
            try {
                exec("SELECT InitSpatialMetaData(1)");
            }
            catch (std::exception const&) {
            }
    }

    db::rowset exec(db::query const& qry) override
    {
        auto ret = db::rowset{};
        auto const txt = qry.text(id_quote(), param_mark());
        auto first = txt.data();
        auto last = first + txt.size();
        auto ps = qry.params();
        auto it = ps.begin();
        while (first != last) {
            ret = {};
            sqlite3_stmt* stmt;
            check(sqlite3_prepare_v2(
                      dbc_.get(), first, int(last - first), &stmt, &first),
                  dbc_);
            auto _ = unique_ptr<sqlite3_stmt, sqlite3_finalize>{stmt};
            int params = sqlite3_bind_parameter_count(stmt);
            for (int i{}; i < params && it != ps.end(); ++i, ++it)
                check(bind_value(stmt, i + 1, *it), dbc_);
            int cols = sqlite3_column_count(stmt);
            ret.columns.resize(cols);
            for (int i{}; i < cols; ++i)
                ret.columns[i] = sqlite3_column_name(stmt, i);
            int ec = sqlite3_step(stmt);
            for (; SQLITE_DONE != ec; ec = sqlite3_step(stmt)) {
                check(ec, dbc_);
                auto& row = ret.rows.emplace_back(cols);
                for (int i{}; i < cols; ++i)
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

}  // namespace boat::sql::sqlite

#endif  // BOAT_SQL_SQLITE_COMMAND_HPP
