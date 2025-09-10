// Andrew Naplavkov

#ifndef BOAT_SQL_ADAPTORS_TEXT_HPP
#define BOAT_SQL_ADAPTORS_TEXT_HPP

#include <boat/sql/detail/adaptors/adaptor.hpp>
#include <boat/sql/detail/utility.hpp>

namespace boat::sql::adaptors {

class text : public adaptor {
protected:
    std::string_view tbl_;
    std::string_view col_;

public:
    bool init(table const& tbl, column const& col) override
    {
        tbl_ = tbl.table_name;
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
                    "nclob",
                    "nvarchar",
                    "varchar"},
                   equal(col.type_name)) ||
               (tbl.dbms_name.contains("mysql") && col.type_name == "text") ||
               (tbl.dbms_name.contains("postgresql") &&
                any({"bpchar", "text"}, equal(col.type_name))) ||
               (tbl.dbms_name.contains("sqlite") &&
                any({"char", "clob", "text"}, within(col.type_name)));
    }

    type migrate(std::string_view dbms) const override
    {
        return {dbms.contains("microsoft sql server") ? "nvarchar" : "varchar",
                250};
    }

    void select(db::query& qry) const override
    {
        qry << db::id{tbl_} << "." << db::id{col_};
    }

    void insert(db::query& qry, pfr::variant var) const override
    {
        qry << std::move(var);
    }
};

}  // namespace boat::sql::adaptors

#endif  // BOAT_SQL_ADAPTORS_TEXT_HPP
