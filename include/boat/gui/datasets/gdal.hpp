// Andrew Naplavkov

#ifndef BOAT_GUI_DATASETS_GDAL_HPP
#define BOAT_GUI_DATASETS_GDAL_HPP

#include <boat/gdal/gil.hpp>
#include <boat/gdal/raster.hpp>
#include <boat/gdal/vector.hpp>
#include <boat/geometry/tile.hpp>
#include <boat/gui/datasets/db.hpp>
#include <boost/gil/extension/io/png.hpp>

namespace boat::gui::datasets {

class gdal : public dataset {
    static constexpr auto raster_name = "_raster";
    std::string file_;
    std::shared_ptr<caches::cache> cache_;
    db vector_;

public:
    gdal(std::string file, std::shared_ptr<caches::cache> const& cache)
        : file_{file}
        , cache_{cache}
        , vector_{[=] {
                      auto ret = std::make_unique<boat::gdal::agent>();
                      ret->dataset = boat::gdal::open(file.data());
                      return ret;
                  },
                  cache}
    {
    }

    std::vector<qualified_name> layers() override
    {
        using namespace boat::gdal;
        auto ret = std::vector<qualified_name>{};
        if (get_or_invoke(cache_.get(), file_, [&] {
                return describe(open(file_.data()).get());
            }))
            ret.push_back(qualified_name{{raster_name}});
        ret.append_range(vector_.layers());
        return ret;
    }

    std::generator<feature> features(  //
        qualified_name lyr,
        geometry::geographic::grid grid,
        double resolution) override
    {
        if (lyr != qualified_name{{raster_name}}) {
            co_yield std::ranges::elements_of(
                vector_.features(std::move(lyr), std::move(grid), resolution));
            co_return;
        }
        using namespace boat::gdal;
        namespace gil = boost::gil;
        namespace qvm = boost::qvm;
        constexpr auto limit = 512;
        auto ptr = dataset_ptr{};
        auto ds = [&] {
            if (!ptr)
                ptr = open(file_.data());
            return ptr.get();
        };
        auto r =
            *get_or_invoke(cache_.get(), file_, [&] { return describe(ds()); });
        auto srs = boost::geometry::srs::epsg{r.epsg};
        auto tiles = geometry::covers(
            grid, resolution, limit, r.width, r.height, r.affine, srs);
        for (auto& t : tiles) {
            auto mbr = boat::geometry::envelope(r.width, r.height, t);
            auto a = mbr.min_corner(), b = mbr.max_corner();
            auto scale = boat::geometry::downscaling(r.width, r.height, t.z);
            auto key = std::tuple{file_, t.z, t.y, t.x};
            auto img = get_or_invoke(cache_.get(), key, [&] {
                auto x = static_cast<int>(a.x());
                auto y = static_cast<int>(a.y());
                auto w = static_cast<int>(b.x() - a.x());
                auto h = static_cast<int>(b.y() - a.y());
                auto ret = make_image(w / scale, h / scale, r.to_colors());
                boost::variant2::visit(
                    [&](auto& v) {
                        image_io(ds(), GF_Read, x, y, w, h, gil::view(v));
                    },
                    ret);
                return ret;
            });
            auto os =
                std::ostringstream{std::ios_base::out | std::ios_base::binary};
            gil::write_view(os, gil::view(img), gil::png_tag());
            auto str = std::move(os).str();
            auto mat =  //
                r.affine * qvm::translation_mat(qvm::vec{{a.x(), a.y()}}) *
                qvm::diag_mat(qvm::vec{{scale * 1., scale * 1., 1.}});
            co_yield feature{
                std::in_place_type<raster>,
                blob{as_bytes(str.data()), str.size()},
                mat,
                r.epsg,
            };
        }
    }
};

}  // namespace boat::gui::datasets

#endif  // BOAT_GUI_DATASETS_GDAL_HPP
