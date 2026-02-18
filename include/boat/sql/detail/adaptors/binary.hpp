// Andrew Naplavkov

#ifndef BOAT_SQL_ADAPTORS_BINARY_HPP
#define BOAT_SQL_ADAPTORS_BINARY_HPP

#include <boat/sql/detail/adaptors/adaptor.hpp>
#include <boat/sql/detail/utility.hpp>

namespace boat::sql::adaptors {

template <>
struct type_name<blob> {
    static constexpr auto value = "blob";
};

class binary : public adaptor {
    std::string_view col_;

public:
    bool init(std::string_view dbms, column const& col) override
    {
        col_ = col.column_name;
        return any({"binary",
                    "binary large object",
                    "binary varying",
                    "blob",
                    "varbinary"},
                   equal(col.lcase_type)) ||
               (dbms.contains(mysql_dbms) && col.lcase_type.contains("blob")) ||
               (dbms.contains(postgresql_dbms) && col.lcase_type == "bytea") ||
               (dbms.contains(sqlite_dbms) && col.lcase_type.contains("blob"));
    }

    type to_type(std::string_view dbms) const override
    {
        return dbms.contains(mssql_dbms)        ? type{"varbinary", -1}
               : dbms.contains(postgresql_dbms) ? type{"bytea"}
                                                : type{"blob"};
    }

    void select(db::query& qry) const override { qry << db::id{col_}; }

    void insert(db::query& qry, pfr::variant var) const override
    {
        qry << std::move(var);
    }
};

}  // namespace boat::sql::adaptors

#endif  // BOAT_SQL_ADAPTORS_BINARY_HPP
