// Andrew Naplavkov

#ifndef BOAT_TEST_GUI_CONTEXT_HPP
#define BOAT_TEST_GUI_CONTEXT_HPP

#include <boat/geometry/map.hpp>
#include <generator>

constexpr auto transparent_ratio = .205;
constexpr auto transparent_tolerance = .01;

struct context {
    int width;
    int height;
    double resolution;
    boost::geometry::srs::proj4 srs;
    boat::geometry::cartesian::box mbr;
    boat::geometry::geographic::grid grid;
};

inline std::generator<context> contexts()
{
    using namespace std::string_literals;
    for (auto [width, height] : {std::pair{1280, 720}, {1920, 1080}})
        for (auto resolution : {500., 10'000.})
            for (auto& ll : boat::geometry::geographic::multi_point{
                     {25., 25.}, {-179., 68.}})
                for (
                    auto& txt : {
                        // clang-format off

boat::concat("+proj=ortho +a=6370997 +b=6370997 +lat_0=", ll.y(), " +lon_0=", ll.x(), " +x_0=0 +y_0=0 +units=m +no_defs"),
"+proj=merc +a=6378137 +b=6378137 +lat_ts=0 +lon_0=0 +x_0=0 +y_0=0 +units=m +no_defs +k=1 +nadgrids=@null +wktext +type=crs"s,

                        // clang-format on
                    }) {
                    auto srs = boost::geometry::srs::proj4{txt};
                    auto pj = boost::geometry::srs::projection<>{srs};
                    auto xy = boat::geometry::cartesian::point{};
                    boat::check(pj.forward(ll, xy), "contexts");
                    auto scale = boat::geometry::scale(pj, ll, resolution);
                    auto mbr =
                        boat::geometry::envelope(xy, scale, width, height);
                    auto num_points = (width * height) / (96 * 96);
                    co_yield {width,
                              height,
                              resolution,
                              srs,
                              mbr,
                              boat::geometry::inverse(pj, mbr, num_points)};
                }
}

#endif  // BOAT_TEST_GUI_CONTEXT_HPP
