// Andrew Naplavkov

#ifndef BOAT_SQL_ADAPTORS_REAL_HPP
#define BOAT_SQL_ADAPTORS_REAL_HPP

#include <boat/sql/detail/adaptors/adaptor.hpp>
#include <boat/sql/detail/utility.hpp>

namespace boat::sql::adaptors {

class real : public adaptor {
protected:
    std::string_view tbl_;
    std::string_view col_;

public:
    bool init(table const& tbl, column const& col) override
    {
        tbl_ = tbl.table_name;
        col_ = col.column_name;
        return any({"decfloat"
                    "decimal",
                    "double precision",
                    "float",
                    "numeric",
                    "real"},
                   equal(col.type_name)) ||
               (tbl.dbms_name.contains("mysql") && col.type_name == "double") ||
               (tbl.dbms_name.contains("sqlite") &&
                any({"real", "floa", "doub"}, within(col.type_name)));
    }

    type migrate(std::string_view dbms) const override
    {
        return {dbms.contains("microsoft sql server") ? "float"
                                                      : "double precision"};
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

#endif  // BOAT_SQL_ADAPTORS_REAL_HPP
