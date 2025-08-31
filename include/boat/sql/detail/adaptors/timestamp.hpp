// Andrew Naplavkov

#ifndef BOAT_SQL_ADAPTORS_TIMESTAMP_HPP
#define BOAT_SQL_ADAPTORS_TIMESTAMP_HPP

#include <boat/sql/detail/adaptors/adaptor.hpp>

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
        return {dbms.contains("microsoft sql server") ? "datetime2"
                                                      : "timestamp"};
    }

    void select(db::query& qry) const override
    {
        if (tbl_->dbms_name.contains("sqlite"))
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
        if (tbl_->dbms_name.contains("sqlite"))
            qry << std::move(var);
        else
            qry << "cast(" << std::move(var) << " as " << col_->type_name
                << ")";
    }
};

}  // namespace boat::sql::adaptors

#endif  // BOAT_SQL_ADAPTORS_TIMESTAMP_HPP
