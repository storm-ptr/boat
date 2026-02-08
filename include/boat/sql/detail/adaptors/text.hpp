// Andrew Naplavkov

#ifndef BOAT_SQL_ADAPTORS_TEXT_HPP
#define BOAT_SQL_ADAPTORS_TEXT_HPP

#include <boat/sql/detail/adaptors/adaptor.hpp>
#include <boat/sql/detail/utility.hpp>

namespace boat::sql::adaptors {

template <>
struct type_name<std::string> {
    static constexpr auto value = "varchar";
};

class text : public adaptor {
    std::string_view col_;

public:
    bool init(std::string_view dbms, column const& col) override
    {
        col_ = col.column_name;
        return any({"char",
                    "character",
                    "character large object",
                    "character varying",
                    "clob",
                    "national character",
                    "national character large object",
                    "national character varying",
                    "nchar",
                    "nchar varying",
                    "nclob",
                    "nvarchar",
                    "varchar"},
                   equal(col.lcase_type)) ||
               (dbms.contains(mysql_dbms) && col.lcase_type.contains("text")) ||
               (dbms.contains(postgresql_dbms) &&
                any({"bpchar", "text"}, equal(col.lcase_type))) ||
               (dbms.contains(sqlite_dbms) &&
                any({"char", "clob", "text"}, within(col.lcase_type)));
    }

    type type_cast(std::string_view dbms) const override
    {
        return {dbms.contains(mssql_dbms) ? "nvarchar" : "varchar", 250};
    }

    void select(db::query& qry) const override { qry << db::id{col_}; }

    void insert(db::query& qry, pfr::variant var) const override
    {
        qry << std::move(var);
    }
};

}  // namespace boat::sql::adaptors

#endif  // BOAT_SQL_ADAPTORS_TEXT_HPP
