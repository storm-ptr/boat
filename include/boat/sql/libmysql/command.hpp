// Andrew Naplavkov

#ifndef BOAT_SQL_LIBMYSQL_COMMAND_HPP
#define BOAT_SQL_LIBMYSQL_COMMAND_HPP

#include <boat/db/command.hpp>
#include <boat/sql/libmysql/detail/fetch.hpp>

namespace boat::sql::libmysql {

class command : public db::command {
    unique_ptr<MYSQL, mysql_close> dbc_;

public:
    command(char const* user,
            char const* password,
            char const* host,
            int port,
            char const* database)
    {
        auto seconds = 30u;
        dbc_.reset(mysql_init(0));
        check(!!dbc_, dbc_);
        for (auto opt : {MYSQL_OPT_CONNECT_TIMEOUT,
                         MYSQL_OPT_READ_TIMEOUT,
                         MYSQL_OPT_WRITE_TIMEOUT})
            check(!mysql_options(dbc_.get(), opt, &seconds), dbc_);
        check(mysql_real_connect(  //
                  dbc_.get(),
                  host,
                  user,
                  password,
                  database,
                  port,
                  0,
                  CLIENT_MULTI_STATEMENTS) == dbc_.get(),
              dbc_);
        check(!mysql_set_character_set(dbc_.get(), "utf8"), dbc_);
    }

    db::rowset exec(db::query const& qry) override
    {
        auto ret = db::rowset{};
        auto txt = qry.text(id_quote(), param_mark());
        auto params = qry.params() | std::views::transform(to_bind) |
                      std::ranges::to<std::vector>();
        if (params.empty()) {
            check(!mysql_query(dbc_.get(), txt.data()), dbc_);
            for (int ec{}; ec >= 0; ec = mysql_next_result(dbc_.get())) {
                check(!ec, dbc_);
                ret = fetch(dbc_.get());
            }
        }
        else {
            auto stmt = unique_ptr<MYSQL_STMT, mysql_stmt_close>{
                mysql_stmt_init(dbc_.get())};
            check(!!stmt, dbc_);
            check(!mysql_stmt_prepare(  //
                      stmt.get(),
                      txt.data(),
                      static_cast<unsigned long>(txt.size())),
                  stmt);
            check(!mysql_stmt_bind_param(stmt.get(), params.data()), stmt);
            check(!mysql_stmt_execute(stmt.get()), stmt);
            for (int ec{}; ec >= 0; ec = mysql_stmt_next_result(stmt.get())) {
                check(!ec, stmt);
                ret = fetch(stmt.get());
            }
        }
        return ret;
    }

    void set_autocommit(bool on) override
    {
        if (on)
            check(!mysql_rollback(dbc_.get()), dbc_);
        check(!mysql_autocommit(dbc_.get(), on), dbc_);
    }

    void commit() override { check(!mysql_commit(dbc_.get()), dbc_); }
    char id_quote() override { return '`'; }
    std::string param_mark() override { return "?"; }
    std::string dbms() override { return "mysql"; }
};

}  // namespace boat::sql::libmysql

#endif  // BOAT_SQL_LIBMYSQL_COMMAND_HPP
