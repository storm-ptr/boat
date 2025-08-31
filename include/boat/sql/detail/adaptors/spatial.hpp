// Andrew Naplavkov

#ifndef BOAT_SQL_ADAPTORS_SPATIAL_HPP
#define BOAT_SQL_ADAPTORS_SPATIAL_HPP

#include <boat/sql/detail/adaptors/adaptor.hpp>

namespace boat::sql::adaptors {

class spatial : public adaptor {
protected:
    table const* tbl_;
    column const* col_;

public:
    bool init(table const& tbl, column const& col) override
    {
        tbl_ = &tbl;
        col_ = &col;
        return col_->srid > 0;
    }

    type migrate(std::string_view dbms) const override
    {
        return dbms.contains("microsoft sql server")
                   ? type{"geography", -1, 4326}
                   : type{"geometry", 0, col_->epsg};
    }

    void select(db::query& qry) const override
    {
        if (tbl_->dbms_name.contains("microsoft sql server"))
            qry << db::id{tbl_->table_name} << "." << db::id{col_->column_name}
                << ".STAsBinary() " << db::id{col_->column_name};
        else
            qry << "ST_AsBinary(" << db::id{tbl_->table_name} << "."
                << db::id{col_->column_name} << ") "
                << db::id{col_->column_name};
    }

    void insert(db::query& qry, pfr::variant var) const override
    {
        if (tbl_->dbms_name.contains("microsoft sql server"))
            qry << col_->type_name << "::STGeomFromWKB(" << std::move(var)
                << ", " << to_chars(col_->srid) << ")";
        else if (tbl_->dbms_name.contains("postgresql") &&
                 col_->type_name == "geography")
            qry << "ST_GeogFromWKB(" << std::move(var) << ")";
        else
            qry << "ST_GeomFromWKB(" << std::move(var) << ", "
                << to_chars(col_->srid) << ")";
    }
};

}  // namespace boat::sql::adaptors

#endif  // BOAT_SQL_ADAPTORS_SPATIAL_HPP
