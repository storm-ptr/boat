// Andrew Naplavkov

#ifndef BOAT_SQL_INSERT_HPP
#define BOAT_SQL_INSERT_HPP

#include <boat/db/query.hpp>
#include <boat/pfr/rowset.hpp>
#include <boat/sql/detail/syntax.hpp>

namespace boat::sql {

inline std::generator<db::query> inserts(table const& tbl,
                                         pfr::rowset const& vals)
{
    auto cols = std::vector<std::unique_ptr<adaptors::adaptor>>{};
    for (auto const& col : vals.columns)
        cols.push_back(adaptors::create(
            tbl, *std::ranges::find(tbl.columns, col, &column::column_name)));
    auto num_rows = std::max<size_t>(1, 999 / cols.size());
    for (auto const& rows : vals | std::views::chunk(num_rows)) {
        auto qry = db::query{};
        qry << "\n insert into " << id{tbl};
        auto sep1 = "\n   (";
        for (auto const& col : vals.columns)
            qry << std::exchange(sep1, ", ") << db::id(col);
        qry << ")\n values";
        sep1 = "\n   ";
        for (auto const& row : rows) {
            qry << std::exchange(sep1, "\n , ");
            auto sep2 = "(";
            for (auto [col, var] : std::views::zip(cols, row)) {
                qry << std::exchange(sep2, ", ");
                col->insert(qry, var);
            }
            qry << ")";
        }
        co_yield std::move(qry);
    }
}

}  // namespace boat::sql

#endif  // BOAT_SQL_INSERT_HPP
