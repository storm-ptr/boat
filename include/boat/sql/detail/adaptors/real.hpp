// Andrew Naplavkov

#ifndef BOAT_SQL_ADAPTORS_REAL_HPP
#define BOAT_SQL_ADAPTORS_REAL_HPP

#include <boat/sql/detail/adaptors/adaptor.hpp>
#include <boat/sql/detail/utility.hpp>

namespace boat::sql::adaptors {

template <std::floating_point T>
struct type_name<T> {
    static constexpr auto value = "double precision";
};

class real : public adaptor {
    std::string_view col_;

public:
    bool init(std::string_view dbms, column const& col) override
    {
        col_ = col.column_name;
        return any({"decfloat",
                    "decimal",
                    "double precision",
                    "float",
                    "numeric",
                    "real"},
                   equal(col.lcase_type)) ||
               (dbms.contains(mysql_dbms) && col.lcase_type == "double") ||
               (dbms.contains(sqlite_dbms) &&
                any({"real", "floa", "doub"}, within(col.lcase_type)));
    }

    type type_cast(std::string_view dbms) const override
    {
        return {dbms.contains(mssql_dbms) ? "float" : "double precision"};
    }

    void select(db::query& qry) const override { qry << db::id{col_}; }

    void insert(db::query& qry, pfr::variant var) const override
    {
        qry << std::move(var);
    }
};

}  // namespace boat::sql::adaptors

#endif  // BOAT_SQL_ADAPTORS_REAL_HPP
