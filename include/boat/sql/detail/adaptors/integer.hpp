// Andrew Naplavkov

#ifndef BOAT_SQL_ADAPTORS_INTEGER_HPP
#define BOAT_SQL_ADAPTORS_INTEGER_HPP

#include <boat/sql/detail/adaptors/adaptor.hpp>

namespace boat::sql::adaptors {

class integer : public adaptor {
protected:
    std::string_view tbl_;
    std::string_view col_;

public:
    bool init(table const& tbl, column const& col) override
    {
        tbl_ = tbl.table_name;
        col_ = col.column_name;
        return any({"bigint", "boolean", "int", "integer", "smallint"},
                   equal(col.type_name)) ||
               (tbl.dbms_name.contains("microsoft sql server") &&
                any({"bit", "tinyint"}, equal(col.type_name))) ||
               (tbl.dbms_name.contains("postgresql") &&
                any({"bigserial", "smallserial", "serial"},
                    equal(col.type_name)));
    }

    type migrate(std::string_view) const override { return {"bigint"}; }

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

#endif  // BOAT_SQL_ADAPTORS_INTEGER_HPP
