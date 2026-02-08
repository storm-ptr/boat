// Andrew Naplavkov

#ifndef BOAT_SQL_ADAPTORS_TIMESTAMP_HPP
#define BOAT_SQL_ADAPTORS_TIMESTAMP_HPP

#include <boat/sql/detail/adaptors/adaptor.hpp>
#include <boat/sql/detail/utility.hpp>
#include <chrono>

namespace boat::sql::adaptors {

template <specialized<std::chrono::time_point> T>
struct type_name<T> {
    static constexpr auto value = "timestamp";
};

class timestamp : public adaptor {
    std::string_view dbms_;
    column const* col_;

public:
    bool init(std::string_view dbms, column const& col) override
    {
        dbms_ = dbms;
        col_ = &col;
        return any({"datetime", "timestamp"}, within(col.lcase_type));
    }

    type type_cast(std::string_view dbms) const override
    {
        return {dbms.contains(mysql_dbms)   ? type{"datetime", 6}
                : dbms.contains(mssql_dbms) ? type{"datetime2"}
                                            : type{"timestamp"}};
    }

    void select(db::query& qry) const override
    {
        if (dbms_.contains(mssql_dbms) && col_->lcase_type == "datetimeoffset")
            qry << "cast(cast(" << db::id{col_->column_name}
                << " at time zone 'UTC' as datetime2) as varchar(50)) "
                << db::id{col_->column_name};
        else if (dbms_.contains(mysql_dbms))
            qry << "date_format(" << db::id{col_->column_name}
                << ", '%Y-%m-%d %H:%i:%s.%f')" << db::id{col_->column_name};
        else if (dbms_.contains(postgresql_dbms) &&
                 col_->lcase_type == "timestamp with time zone")
            qry << "cast(cast(" << db::id{col_->column_name}
                << " at time zone 'UTC' as timestamp) as varchar(50)) "
                << db::id{col_->column_name};
        else if (dbms_.contains(sqlite_dbms))
            qry << "datetime(" << db::id{col_->column_name} << ", 'subsec') "
                << db::id{col_->column_name};
        else
            qry << "cast(" << db::id{col_->column_name} << " as varchar(50)) "
                << db::id{col_->column_name};
    }

    void insert(db::query& qry, pfr::variant var) const override
    {
        if (dbms_.contains(mysql_dbms))
            qry << "str_to_date(" << std::move(var)
                << ", '%Y-%m-%d %H:%i:%s.%f')";
        else if (dbms_.contains(sqlite_dbms))
            qry << std::move(var);
        else
            qry << "cast(" << std::move(var) << " as " << col_->lcase_type
                << ")";
    }
};

}  // namespace boat::sql::adaptors

#endif  // BOAT_SQL_ADAPTORS_TIMESTAMP_HPP
