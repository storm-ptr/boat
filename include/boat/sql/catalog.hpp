// Andrew Naplavkov

#ifndef BOAT_SQL_CATALOG_HPP
#define BOAT_SQL_CATALOG_HPP

#include <boat/db/catalog.hpp>
#include <boat/sql/detail/exec.hpp>

namespace boat::sql {

/// Raster operations throw std::logic_error.
class catalog : public db::catalog {
    inline static auto err = std::logic_error{"sql"};

    auto& dial() { return dialects::find(command->dbms()); }

public:
    std::unique_ptr<db::command> command;

    std::vector<db::source> sources() override { return {}; }

    std::vector<db::layer> layers() override
    {
        return {
            std::from_range,
            command->exec(dial().layers()) | db::view<db::layer>};
    }

    db::table get_table(
        std::string_view schema_name,
        std::string_view table_name) override
    {
        return sql::get_table(*command, schema_name, table_name);
    }

    db::rowset select(db::table const& tbl, db::page const& rq) override
    {
        auto q = db::query{};
        q << "\n select " << select_list{tbl, rq.select_list} << "\n from "
          << id{tbl} << order_by{tbl, rq.order_by} << "\n limit "
          << to_chars(rq.limit) << "\n offset " << to_chars(rq.offset);
        return command->exec(q);
    }

    db::rowset select(db::table const& tbl, db::bbox const& rq) override
    {
        return command->exec(dial().select(tbl, rq));
    }

    void insert(
        db::table const& tbl,
        db::rowset const& rs,
        std::stop_token tok = {}) override
    {
        if (rs.empty())
            return;
        check(!rs.columns.empty(), "no columns");
        auto cols = std::vector<std::unique_ptr<adaptors::adaptor>>{};
        for (auto& col : rs.columns)
            cols.push_back(adaptors::make(tbl.dbms, find(tbl.columns, col)));
        for (auto&& rows :
             rs | std::views::chunk(std::max<>(1uz, 999uz / cols.size()))) {
            if (tok.stop_requested())
                break;
            auto q = db::query{"\n insert into ", id{tbl}};
            for (auto sep{"\n   ("}; auto& col : rs.columns)
                q << std::exchange(sep, ", ") << db::id(col);
            for (auto sep1{")\n values\n   "}; auto& row : rows) {
                q << std::exchange(sep1, "\n , ");
                for (auto sep2{"("}; auto [c, v] : std::views::zip(cols, row)) {
                    q << std::exchange(sep2, ", ");
                    c->insert(q, v);
                }
                q << ")";
            }
            command->exec(q, tok);
        }
    }

    /// Adapts column types and adds missing spatial indexes.
    db::table create(db::table const& tbl) override
    {
        auto t = migrate(*command, tbl);
        command->exec(dial().create(t));
        return get_table(t.schema_name, t.table_name);
    }

    void drop(std::string_view schema_name, std::string_view table_name)
        override
    {
        auto scm = current_schema_or(*command, schema_name);
        command->exec({"drop table if exists ", id{scm, table_name}});
    }

    db::raster get_raster(db::layer const&) override { throw err; }

    std::generator<std::pair<tile, gil::any_image>> read(
        db::raster,
        std::vector<tile>) override
    {
        throw err;
        co_return;
    }

    void write(db::raster const&, db::rect const&, gil::any_image_view) override
    {
        throw err;
    }

    void set_autocommit(bool on) override { command->set_autocommit(on); }

    void commit() override { command->commit(); }
};

}  // namespace boat::sql

#endif  // BOAT_SQL_CATALOG_HPP
