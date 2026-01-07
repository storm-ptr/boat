// Andrew Naplavkov

#ifndef BOAT_DB_LIBPQ_COMMAND_HPP
#define BOAT_DB_LIBPQ_COMMAND_HPP

#include <boat/db/command.hpp>
#include <boat/db/libpq/detail/get_value.hpp>
#include <boat/db/libpq/detail/params.hpp>

namespace boat::db::libpq {

class command : public db::command {
    unique_ptr<PGconn, PQfinish> dbc_;

public:
    explicit command(char const* connection) : dbc_(PQconnectdb(connection))
    {
        check(dbc_ && PQstatus(dbc_.get()) == CONNECTION_OK, dbc_);
    }

    pfr::rowset exec(query const& qry) override
    {
        auto ret = pfr::rowset{};
        auto sql = qry.sql(id_quote(), param_mark());
        auto binds = std::vector<std::unique_ptr<params::param>>{};
        auto types = std::vector<Oid>{};
        auto values = std::vector<char const*>{};
        auto lengths = std::vector<int>{};
        auto formats = std::vector<int>{};
        for (auto var : qry.params()) {
            auto bnd = binds.emplace_back(params::create(var)).get();
            types.emplace_back(bnd->type());
            values.emplace_back(bnd->value());
            lengths.emplace_back(bnd->length());
            formats.emplace_back(bnd->format());
        }
        auto res = unique_ptr<PGresult, PQclear>{
            binds.empty() ? PQexec(dbc_.get(), sql.data())
                          : PQexecParams(  //
                                dbc_.get(),
                                sql.data(),
                                int(binds.size()),
                                types.data(),
                                values.data(),
                                lengths.data(),
                                formats.data(),
                                text_fmt)};
        auto ec = PQresultStatus(res.get());
        check(ec == PGRES_COMMAND_OK || ec == PGRES_TUPLES_OK, dbc_);
        ret.columns.resize(res ? PQnfields(res.get()) : 0);
        ret.rows.resize(res ? PQntuples(res.get()) : 0);
        for (int col{}; col < static_cast<int>(ret.columns.size()); ++col)
            ret.columns[col] = PQfname(res.get(), col);
        for (int row{}; row < static_cast<int>(ret.rows.size()); ++row) {
            ret.rows[row].resize(ret.columns.size());
            for (int col{}; col < static_cast<int>(ret.columns.size()); ++col)
                ret.rows[row][col] = get_value(res.get(), row, col);
        }
        return ret;
    }

    void set_autocommit(bool on) override { exec(on ? "rollback;" : "begin;"); }
    void commit() override { exec("commit;begin;"); }
    char id_quote() override { return '"'; }
    std::string param_mark() override { return "${}"; }
    std::string dbms() override { return "postgresql"; }
};

}  // namespace boat::db::libpq

#endif  // BOAT_DB_LIBPQ_COMMAND_HPP
