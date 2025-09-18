// Andrew Naplavkov

#ifndef BOAT_DB_ODBC_COMMAND_HPP
#define BOAT_DB_ODBC_COMMAND_HPP

#include <boat/db/command.hpp>
#include <boat/db/odbc/detail/get_data.hpp>
#include <boat/db/odbc/detail/params.hpp>

namespace boat::db::odbc {

class command : public db::command {
    env_ptr env_;
    dbc_ptr dbc_;

public:
    explicit command(std::string_view connection)
    {
        env_ = alloc<SQL_HANDLE_ENV>(env_);
        check(
            SQLSetEnvAttr(
                env_.get(), SQL_ATTR_ODBC_VERSION, SQLPOINTER(SQL_OV_ODBC3), 0),
            env_);
        dbc_ = alloc<SQL_HANDLE_DBC>(env_);
        SQLSMALLINT len;
        check(SQLDriverConnectW(dbc_.get(),
                                0,
                                unicode::string<SQLWCHAR>(connection).data(),
                                SQL_NTS,
                                0,
                                0,
                                &len,
                                SQL_DRIVER_NOPROMPT),
              dbc_);
    }

    pfr::rowset exec(query const& qry) override
    {
        auto ret = pfr::rowset{};
        auto stmt = alloc<SQL_HANDLE_STMT>(dbc_);
        auto sql =
            qry.sql(id_quote(), param_mark()) | unicode::string<SQLWCHAR>;
        check(SQLPrepareW(stmt.get(), sql.data(), SQL_NTS), stmt);
        auto binds = std::vector<std::unique_ptr<params::param>>{};
        for (auto [i, var] : qry.params() | std::views::enumerate) {
            auto bnd = binds.emplace_back(params::create(var)).get();
            check(SQLBindParameter(stmt.get(),
                                   SQLUSMALLINT(i + 1),
                                   SQL_PARAM_INPUT,
                                   bnd->c_type(),
                                   bnd->sql_type(),
                                   bnd->length(),
                                   0,
                                   bnd->value(),
                                   0,
                                   bnd->indicator()),
                  stmt);
        }
        auto rc = SQLExecute(stmt.get());
        for (; SQL_NO_DATA != rc; rc = SQLMoreResults(stmt.get())) {
            check(rc, stmt);
            ret = {};
            SQLSMALLINT num_cols;
            check(SQLNumResultCols(stmt.get(), &num_cols), stmt);
            if (num_cols) {
                for (int i{}; i < num_cols; ++i)
                    ret.columns.push_back(name(stmt, i + 1));
                auto rdr = reader{};
                rc = SQLFetch(stmt.get());
                for (; SQL_NO_DATA != rc; rc = SQLFetch(stmt.get())) {
                    check(rc, stmt);
                    auto& row = ret.rows.emplace_back(num_cols);
                    for (int i{}; i < num_cols; ++i)
                        row[i] = rdr.get_data(stmt, i + 1);
                }
            }
        }
        return ret;
    }

    void set_autocommit(bool on) override
    {
        if (on)
            check(SQLEndTran(SQL_HANDLE_DBC, dbc_.get(), SQL_ROLLBACK), dbc_);
        intptr_t val = on ? SQL_AUTOCOMMIT_ON : SQL_AUTOCOMMIT_OFF;
        auto ptr = SQLPOINTER(val);
        check(SQLSetConnectAttr(dbc_.get(), SQL_ATTR_AUTOCOMMIT, ptr, 0), dbc_);
    }

    void commit() override
    {
        check(SQLEndTran(SQL_HANDLE_DBC, dbc_.get(), SQL_COMMIT), dbc_);
    }

    char id_quote() override
    {
        return info(dbc_, SQL_IDENTIFIER_QUOTE_CHAR).at(0);
    }

    std::string param_mark() override { return "?"; }
    std::string dbms() override { return info(dbc_, SQL_DBMS_NAME); }
};

}  // namespace boat::db::odbc

#endif  // BOAT_DB_ODBC_COMMAND_HPP
