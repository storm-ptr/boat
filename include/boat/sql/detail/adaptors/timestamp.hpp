// Andrew Naplavkov

#ifndef BOAT_SQL_ADAPTORS_TIMESTAMP_HPP
#define BOAT_SQL_ADAPTORS_TIMESTAMP_HPP

#include <boat/sql/detail/adaptors/adaptor.hpp>
#include <boat/sql/detail/utility.hpp>

namespace boat::sql::adaptors {

struct timestamp : impl<std::chrono::sys_seconds> {
    std::string_view parse() const override
    {
        return any({"datetime", "timestamp"}, in(type())) ? kind_ : "";
    }

    db::column migrate(std::string_view dbms) const override
    {
        return {.kind{kind_},
                .column_name{col_->column_name},
                .type_name{is_mssql(dbms)   ? "datetime2"
                           : is_mysql(dbms) ? "datetime"
                                            : "timestamp"},
                .length = is_mysql(dbms) ? 6 : 0};
    }

    void select(db::query& qry) const override
    {
        auto id = db::id{col_->column_name};
        if (is_mssql(dbms_) && type() == "datetimeoffset")
            qry << "cast(cast(" << id
                << " at time zone 'UTC' as datetime2) as varchar(50)) " << id;
        else if (is_mysql(dbms_))
            qry << "date_format(" << id << ", '%Y-%m-%d %H:%i:%s.%f')" << id;
        else if (is_postgres(dbms_) && type() == "timestamp with time zone")
            qry << "cast(cast(" << id
                << " at time zone 'UTC' as timestamp) as varchar(50)) " << id;
        else if (is_sqlite(dbms_))
            qry << "datetime(" << id << ", 'subsec') " << id;
        else
            qry << "cast(" << id << " as varchar(50)) " << id;
    }

    void insert(db::query& qry, db::variant var) const override
    {
        if (is_mysql(dbms_))
            qry << "str_to_date(" << std::move(var)
                << ", '%Y-%m-%d %H:%i:%s.%f')";
        else if (is_sqlite(dbms_))
            qry << std::move(var);
        else
            qry << "cast(" << std::move(var) << " as " << type() << ")";
    }
};

}  // namespace boat::sql::adaptors

#endif  // BOAT_SQL_ADAPTORS_TIMESTAMP_HPP
