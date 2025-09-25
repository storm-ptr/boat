// Andrew Naplavkov

#ifndef BOAT_GUI_DATASETS_SQL_HPP
#define BOAT_GUI_DATASETS_SQL_HPP

#include <boat/db/commands.hpp>
#include <boat/geometry/transform.hpp>
#include <boat/gui/datasets/dataset.hpp>
#include <boat/sql/api.hpp>

namespace boat::gui::datasets {

class sql : public dataset {
    using command_fn = std::move_only_function<std::unique_ptr<db::command>()>;
    command_fn cmd_;
    std::shared_ptr<caches::cache> cache_;
    size_t key_;

public:
    sql(command_fn cmd, std::shared_ptr<caches::cache> const& cache)
        : cmd_(std::move(cmd)), cache_(cache), key_(caches::next_key())
    {
    }

    sql(std::string url, std::shared_ptr<caches::cache> const& cache)
        : sql([url = std::move(url)] { return db::create(url); }, cache)
    {
    }

    std::vector<layer> layers() override
    {
        return get_or_invoke(cache_.get(), key_, [&] {
            return boat::sql::get_layers(*cmd_()) |
                   std::views::transform([](auto& l) -> layer {
                       return {l.schema_name, l.table_name, l.column_name};
                   }) |
                   std::ranges::to<std::vector>();
        });
    }

    std::generator<feature> features(layer lyr,
                                     geometry::geographic::grid grid,
                                     double) override
    {
        using namespace boat::sql;
        namespace bgi = boost::geometry::index;
        namespace bgs = boost::geometry::srs;
        auto cmd = cmd_();
        auto tbl = get_or_invoke(cache_.get(), std::tuple{key_, lyr}, [&] {
            return get_table(*cmd, lyr.at(0), lyr.at(1));
        });
        auto& col = lyr.at(2);
        auto it = std::ranges::find(tbl.columns, col, &column::column_name);
        check(it != tbl.columns.end(), col);
        auto tf = bgs::transformation<>(bgs::epsg{4326}, bgs::epsg{it->epsg});
        auto fwd = geometry::transformer(geometry::forwarder(tf));
        auto voids = bgi::rtree<geometry::geographic::box, bgi::rstar<4>>{};
        auto processed = std::unordered_set<pfr::variant>{};
        for (auto& lvl : grid | std::views::reverse) {
            auto buf = geometry::buffer(lvl.first / 2, 4);
            for (auto& p1 : lvl.second) {
                auto b1 = geometry::envelope(buf(p1));
                if (!boost::geometry::within(p1, b1))
                    continue;
                if (bgi::qbegin(voids, bgi::contains(b1)) != bgi::qend(voids))
                    continue;
                auto b2 = fwd(b1);
                if (!b2)
                    continue;
                auto a = b2->min_corner(), b = b2->max_corner();
                auto key = std::tuple{key_, lyr, a.x(), a.y(), b.x(), b.y()};
                auto rows = get_or_invoke(cache_.get(), key, [&] {
                    auto req = box{{col}, col, a.x(), a.y(), b.x(), b.y(), 1};
                    return select(*cmd, tbl, req);
                });
                if (rows.empty()) {
                    voids.insert(b1);
                    continue;
                }
                auto& var = rows.value();
                if (!processed.insert(var).second)
                    continue;
                co_yield {{}, std::get<blob>(var), it->epsg};
            }
        }
    }
};

}  // namespace boat::gui::datasets

#endif  // BOAT_GUI_DATASETS_SQL_HPP
