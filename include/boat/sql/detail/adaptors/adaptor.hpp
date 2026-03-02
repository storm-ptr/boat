// Andrew Naplavkov

#ifndef BOAT_SQL_ADAPTORS_ADAPTOR_HPP
#define BOAT_SQL_ADAPTORS_ADAPTOR_HPP

#include <boat/db/meta.hpp>
#include <boat/db/query.hpp>

namespace boat::sql::adaptors {

struct adaptor {
    virtual ~adaptor() = default;
    virtual std::string_view parse() const = 0;
    virtual db::column migrate(std::string_view dbms) const = 0;
    virtual void select(db::query&) const = 0;
    virtual void insert(db::query&, db::variant) const = 0;
};

template <class T>
class impl : public adaptor {
protected:
    static constexpr auto kind_ = db::kind<T>::value;

    std::string_view dbms_;
    db::column const* col_;

    std::string_view type() const { return col_->type_name; }

public:
    bool init(std::string_view dbms, db::column const& col)
    {
        dbms_ = dbms;
        col_ = &col;
        return col.kind == kind_;
    }

    void select(db::query& qry) const override
    {
        qry << db::id{col_->column_name};
    }

    void insert(db::query& qry, db::variant var) const override
    {
        qry << std::move(var);
    }
};

}  // namespace boat::sql::adaptors

#endif  // BOAT_SQL_ADAPTORS_ADAPTOR_HPP
