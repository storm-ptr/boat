// Andrew Naplavkov

#ifndef BOAT_GUI_DATASETS_DB_HPP
#define BOAT_GUI_DATASETS_DB_HPP

#include <boat/db/agent.hpp>
#include <boat/geometry/transform.hpp>
#include <boat/gui/datasets/dataset.hpp>

namespace boat::gui::datasets {

class db : public dataset {
    using agent_fn =
        std::move_only_function<std::unique_ptr<boat::db::agent>()>;
    agent_fn agt_;
    std::shared_ptr<caches::cache> cache_;
    size_t key_;

public:
    db(agent_fn agt, std::shared_ptr<caches::cache> const& cache)
        : agt_(std::move(agt)), cache_(cache), key_(caches::next_key())
    {
    }

    std::vector<qualified_name> layers() override
    {
        return get_or_invoke(cache_.get(), key_, [&] {
            return agt_()->get_layers() |
                   std::views::transform([](auto& l) -> qualified_name {
                       return {l.schema_name, l.table_name, l.column_name};
                   }) |
                   std::ranges::to<std::vector>();
        });
    }

    std::generator<feature> features(  //
        qualified_name lyr,
        geometry::geographic::grid grid,
        double) override
    {
        namespace bgi = boost::geometry::index;
        auto agt = agt_();
        auto tbl = get_or_invoke(cache_.get(), std::tuple{key_, lyr}, [&] {
            return agt->describe(lyr.at(0), lyr.at(1));
        });
        auto& col = lyr.at(2);
        auto it =
            std::ranges::find(tbl.columns, col, &boat::db::column::column_name);
        check(it != tbl.columns.end(), col);
        auto fwd = geometry::transform(geometry::srs_forward(
            geometry::transformation(boost::geometry::srs::epsg{it->epsg})));
        auto voids = bgi::rtree<geometry::geographic::box, bgi::rstar<4>>{};
        auto processed = std::unordered_set<boat::db::variant>{};
        for (auto& lvl : grid | std::views::reverse) {
            auto buf = geometry::buffer(lvl.first / 2, 4);
            for (auto& p1 : lvl.second) {
                auto b1 = geometry::minmax(buf(p1));
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
                    return agt->select(
                        tbl,
                        boat::db::bbox{
                            {col}, col, a.x(), a.y(), b.x(), b.y(), 1});
                });
                if (rows.empty()) {
                    voids.insert(b1);
                    continue;
                }
                auto& var = rows.value();
                if (!processed.insert(var).second)
                    continue;
                co_yield feature{
                    std::in_place_type<shape>, std::get<blob>(var), it->epsg};
            }
        }
    }
};

}  // namespace boat::gui::datasets

#endif  // BOAT_GUI_DATASETS_DB_HPP
