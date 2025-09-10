// Andrew Naplavkov

#ifndef BOAT_SQL_ADAPTORS_TIMESTAMP_HPP
#define BOAT_SQL_ADAPTORS_TIMESTAMP_HPP

#include <boat/sql/detail/adaptors/adaptor.hpp>
#include <boat/sql/detail/utility.hpp>

namespace boat::sql::adaptors {

class timestamp : public adaptor {
protected:
    table const* tbl_;
    column const* col_;

public:
    bool init(table const& tbl, column const& col) override
    {
        tbl_ = &tbl;
        col_ = &col;
        return any({"datetime", "timestamp"}, within(col.type_name));
    }

    type migrate(std::string_view dbms) const override
    {
        return {dbms.contains("mysql")                  ? "datetime"
                : dbms.contains("microsoft sql server") ? "datetime2"
                                                        : "timestamp"};
    }

    void select(db::query& qry) const override
    {
        if (tbl_->dbms_name.contains("mysql"))
            qry << "cast(" << db::id{tbl_->table_name} << "."
                << db::id{col_->column_name} << " as char) "
                << db::id{col_->column_name};
        else if (tbl_->dbms_name.contains("sqlite"))
            qry << "datetime(" << db::id{tbl_->table_name} << "."
                << db::id{col_->column_name} << ", 'subsec') "
                << db::id{col_->column_name};
        else
            qry << "cast(" << db::id{tbl_->table_name} << "."
                << db::id{col_->column_name} << " as varchar(50)) "
                << db::id{col_->column_name};
    }

    void insert(db::query& qry, pfr::variant var) const override
    {
        if (tbl_->dbms_name.contains("mysql"))
            qry << "str_to_date(" << std::move(var)
                << ", get_format(datetime, 'ISO'))";
        else if (tbl_->dbms_name.contains("sqlite"))
            qry << std::move(var);
        else
            qry << "cast(" << std::move(var) << " as " << col_->type_name
                << ")";
    }
};

}  // namespace boat::sql::adaptors

#endif  // BOAT_SQL_ADAPTORS_TIMESTAMP_HPP
