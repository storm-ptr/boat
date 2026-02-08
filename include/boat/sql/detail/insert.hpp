// Andrew Naplavkov

#ifndef BOAT_SQL_INSERT_HPP
#define BOAT_SQL_INSERT_HPP

#include <boat/pfr/rowset.hpp>
#include <boat/sql/detail/manip.hpp>

namespace boat::sql {

inline std::generator<db::query> inserts(  //
    table const& tbl,
    pfr::rowset const& vals)
{
    auto cols = std::vector<std::unique_ptr<adaptors::adaptor>>{};
    for (auto const& col : vals.columns)
        cols.push_back(adaptors::create(
            tbl.lcase_dbms,
            *std::ranges::find(tbl.columns, col, &column::column_name)));
    auto num_rows = std::max<size_t>(1, 999 / cols.size());
    for (auto const& rows : vals | std::views::chunk(num_rows)) {
        auto qry = db::query{};
        qry << "\n insert into " << id{tbl};
        for (auto sep = "\n   ("; auto const& col : vals.columns)
            qry << std::exchange(sep, ", ") << db::id(col);
        qry << ")\n values";
        for (auto sep1 = "\n   "; auto const& row : rows) {
            qry << std::exchange(sep1, "\n , ");
            for (auto sep2 = "("; auto [col, v] : std::views::zip(cols, row)) {
                qry << std::exchange(sep2, ", ");
                col->insert(qry, v);
            }
            qry << ")";
        }
        co_yield std::move(qry);
    }
}

}  // namespace boat::sql

#endif  // BOAT_SQL_INSERT_HPP
