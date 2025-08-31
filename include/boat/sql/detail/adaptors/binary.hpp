// Andrew Naplavkov

#ifndef BOAT_SQL_ADAPTORS_BINARY_HPP
#define BOAT_SQL_ADAPTORS_BINARY_HPP

#include <boat/sql/detail/adaptors/adaptor.hpp>

namespace boat::sql::adaptors {

class binary : public adaptor {
protected:
    std::string_view tbl_;
    std::string_view col_;

public:
    bool init(table const& tbl, column const& col) override
    {
        tbl_ = tbl.table_name;
        col_ = col.column_name;
        return any({"binary",
                    "binary large object",
                    "binary varying",
                    "blob",
                    "varbinary"},
                   equal(col.type_name)) ||
               (tbl.dbms_name.contains("postgresql") &&
                col.type_name == "bytea") ||
               (tbl.dbms_name.contains("sqlite") &&
                col.type_name.contains("blob"));
    }

    type migrate(std::string_view dbms) const override
    {
        return dbms.contains("microsoft sql server") ? type{"varbinary", -1}
               : dbms.contains("postgresql")         ? type{"bytea"}
                                                     : type{"blob"};
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

#endif  // BOAT_SQL_ADAPTORS_BINARY_HPP
