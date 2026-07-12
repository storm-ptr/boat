// Andrew Naplavkov

#ifndef BOAT_GDAL_COMMAND_HPP
#define BOAT_GDAL_COMMAND_HPP

#include <boat/db/command.hpp>
#include <boat/gdal/detail/vector.hpp>

namespace boat::gdal {

struct command : db::command {
    dataset_ptr dataset;
    std::string dialect;

    db::rowset exec(db::query const& qry) override
    {
        auto txt = qry.text(id_quote(), param_mark());
        if (auto lyr = execute(dataset.get(), txt.data(), dialect.data()))
            return select(lyr.get(), fields::make(lyr.get()), INT_MAX);
        auto err = error_or("");
        return err.empty() ? db::rowset{} : throw std::runtime_error(err);
    }

    void set_autocommit(bool on) override
    {
        gdal::set_autocommit(dataset.get(), on);
    }

    void commit() override { gdal::commit(dataset.get()); }
    char id_quote() override { return '"'; }
    std::string param_mark() override { return {}; }
    std::string dbms() override { return to_lower(dialect); }
};

}  // namespace boat::gdal

#endif  // BOAT_GDAL_COMMAND_HPP
