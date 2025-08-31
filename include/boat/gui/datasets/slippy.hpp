// Andrew Naplavkov

#ifndef BOAT_GUI_DATASETS_SLIPPY_HPP
#define BOAT_GUI_DATASETS_SLIPPY_HPP

#include <boat/detail/curl.hpp>
#include <boat/geometry/transform.hpp>
#include <boat/gui/datasets/dataset.hpp>
#include <boat/slippy/geometry.hpp>
#include <boat/slippy/layers/layers.hpp>
#include <boost/geometry/srs/epsg.hpp>

namespace boat::gui::datasets {

class slippy : public dataset {
    std::string usr_;
    std::shared_ptr<caches::cache> cache_;

public:
    slippy(std::string useragent, std::shared_ptr<caches::cache> const& cache)
        : usr_(std::move(useragent)), cache_(std::move(cache))
    {
    }

    std::vector<layer> layers() override
    {
        auto ret = std::vector<layer>{};
        boost::fusion::for_each(boat::slippy::layers::all, [&](auto& lyr) {
            auto& item = ret.emplace_back();
            item.emplace_back(lyr.company_name());
            item.emplace_back(lyr.layer_name());
        });
        return ret;
    }

    std::generator<feature> features(layer lyr,
                                     geometry::geographic::grid grid,
                                     double resolution) override
    {
        using namespace boat::slippy;
        constexpr auto epsg = 3857;
        static auto const fwd = geometry::transform(
            geometry::forward(boost::geometry::srs::projection<
                              boost::geometry::srs::static_epsg<epsg>>{}));
        auto& urls = layers::find(lyr.at(0), lyr.at(1));
        auto tiles = std::unordered_map<std::string, tile>{};
        auto queue = curl{usr_};
        for (auto t : to_tiles(grid, resolution)) {
            auto url = urls.url(t);
            auto img = cache_ ? cache_->get(url) : std::any{};
            if (img.has_value())
                co_yield {std::any_cast<blob>(std::move(img)),
                          *fwd(geometry::to_polygon(envelope(t))),
                          epsg};
            else {
                tiles.insert({url, t});
                queue.push(url);
            }
        }
        while (queue.size()) {
            auto [url, img] = queue.pop();
            if (cache_)
                cache_->put(url, img);
            co_yield {std::move(img),
                      *fwd(geometry::to_polygon(envelope(tiles.at(url)))),
                      epsg};
        }
    }
};

}  // namespace boat::gui::datasets

#endif  // BOAT_GUI_DATASETS_SLIPPY_HPP
