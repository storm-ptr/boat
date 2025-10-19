// Andrew Naplavkov

#ifndef BOAT_TEST_GUI_CONTEXT_HPP
#define BOAT_TEST_GUI_CONTEXT_HPP

#include <boat/geometry/map.hpp>
#include <boost/qvm/map_vec_mat.hpp>
#include <generator>

constexpr auto transparent_ratio = .2439;
constexpr auto transparent_tolerance = .01;

struct context {
    int width;
    int height;
    double resolution;
    boost::geometry::srs::proj4 srs;
    boost::qvm::mat<double, 3, 3> affine;
    boat::geometry::geographic::grid grid;
};

inline std::generator<context> contexts()
{
    using namespace std::string_literals;
    for (auto ll : {boat::geometry::geographic::point{25., 25.}, {-179., 68.}})
        for (auto [width, height] : {std::pair{1280, 720}, {1920, 1080}})
            for (auto resolution : {500., 10'000.})
                for (auto deg : {0., 10., 45.})
                    for (
                        auto& proj4 : {
                            // clang-format off

boat::concat("+proj=ortho +a=6370997 +b=6370997 +lat_0=", ll.y(), " +lon_0=", ll.x(), " +x_0=0 +y_0=0 +units=m +no_defs"),
"+proj=merc +a=6378137 +b=6378137 +lat_ts=0 +lon_0=0 +x_0=0 +y_0=0 +units=m +no_defs +k=1 +nadgrids=@null +wktext +type=crs"s,
"+proj=longlat +datum=WGS84 +no_defs +type=crs"s,

                            // clang-format on
                        }) {
                        auto srs = boost::geometry::srs::proj4{proj4};
                        auto mbr = boat::geometry::forward(
                            srs, ll, resolution, width, height);
                        BOOST_CHECK(mbr);
                        auto rad = deg * boost::math::double_constants::degree;
                        auto c = boost::geometry::return_centroid<
                            boat::geometry::cartesian::point>(*mbr);
                        auto vec = boost::qvm::vec<double, 2>{c.x(), c.y()};
                        auto mat = boost::qvm::identity_mat<double, 3>() *
                                   boost::qvm::translation_mat(vec) *
                                   boost::qvm::rotz_mat<3>(rad) *
                                   boost::qvm::translation_mat(-vec) *
                                   boat::geometry::affine(*mbr, width, height);
                        auto num_points = (width * height) / (96 * 96);
                        co_yield {
                            width,
                            height,
                            resolution,
                            srs,
                            mat,
                            boat::geometry::inverse(srs, *mbr, num_points)};
                    }
}

#endif  // BOAT_TEST_GUI_CONTEXT_HPP
