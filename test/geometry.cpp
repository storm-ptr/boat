// Andrew Naplavkov

#include <boat/geometry/raster.hpp>
#include <boat/geometry/wkb.hpp>
#include <boost/algorithm/hex.hpp>
#include <boost/fusion/algorithm.hpp>
#include <boost/fusion/container.hpp>
#include <boost/test/unit_test.hpp>
#include "utility.hpp"

using namespace boat::geometry;
using namespace boost::geometry;
using namespace std::literals;

namespace {

auto crs = srs::epsg{3857};
auto tf = transformation(crs);
auto width = 1920;
auto height = 1080;

template <ogc Geom1, same_tag<Geom1> Geom2>
void check(std::string const& wkt)
{
    auto ll = from_wkt<Geom1>(wkt);
    auto xy = Geom2{};
    BOOST_CHECK(transform(ll, xy, srs_forward(tf)));
    BOOST_CHECK_NE(wkt, to_wkt(xy));
    auto wkb = boat::blob{} << xy;
    boat::blob_view{wkb} >> xy;
    BOOST_CHECK(transform(xy, ll, srs_inverse(tf)));
    BOOST_CHECK_EQUAL(wkt, to_wkt(ll));
}

}  // namespace

BOOST_AUTO_TEST_CASE(geometry_transform_wkb)
{
    // clang-format off
    auto geoms = boost::fusion::make_map<
        geographic::point,
        geographic::linestring,
        geographic::polygon,
        geographic::multi_point,
        geographic::multi_linestring,
        geographic::multi_polygon,
        geographic::geometry_collection>(
        "POINT(30 10)"s,
        "LINESTRING(30 10,10 30,40 40)"s,
        "POLYGON((35 10,45 45,15 40,10 20,35 10),(20 30,35 35,30 20,20 30))"s,
        "MULTIPOINT((10 40),(40 30),(20 20),(30 10))"s,
        "MULTILINESTRING((10 10,20 20,10 40),(40 40,30 30,40 20,30 10))"s,
        "MULTIPOLYGON(((40 40,20 45,45 30,40 40)),((20 35,10 30,10 10,30 5,45 20,20 35),(30 20,20 15,20 25,30 20)))"s,
        "GEOMETRYCOLLECTION(POINT(40 10),LINESTRING(10 10,20 20,10 40),POLYGON((40 40,20 45,45 30,40 40)))"s
    );
    // clang-format on
    boost::fusion::for_each(geoms, [&]<class P>(P const& pair) {
        using geom1 = P::first_type;
        using geom2 = std::
            variant_alternative_t<variant_index_v<geom1>, cartesian::variant>;
        auto const& wkt = pair.second;
        check<geom1, geom2>(wkt);
        check<geographic::variant, cartesian::variant>(wkt);
    });
}

BOOST_AUTO_TEST_CASE(geometry_endian)
{
    std::pair<std::string, std::string> tests[] = {
        {"000000000140000000000000004010000000000000", "POINT(2 4)"},
        {"010100000000000000000000400000000000001040", "POINT(2 4)"}};
    for (auto& [hex, wkt] : tests) {
        auto unhex = boost::algorithm::unhex(hex);
        auto in = boat::blob_view{std::as_bytes(std::span(unhex))};
        auto geom = boat::get<cartesian::variant>(in);
        BOOST_CHECK_EQUAL(wkt, to_wkt(geom));
    }
}

BOOST_AUTO_TEST_CASE(geometry_fibonacci_monotonic)
{
    auto lim = 50;
    for (auto fib : fibonacci_levels | std::views::take(18))
        for (auto p : geographic_random() | std::views::take(lim)) {
            auto prev = 0.;
            auto indices = std::unordered_set<size_t>{};
            for (auto [i, j] : fib.nearests(p) | std::views::take(lim) |
                                   std::views::enumerate) {
                auto d = distance(p, fib[j]);
                BOOST_TEST(
                    prev <= d, wkt(p) << ", " << i << "/" << fib.num_points);
                prev = d;
                indices.insert(j);
            }
            auto expect = std::min<size_t>(lim, fib.num_points);
            BOOST_CHECK_EQUAL(indices.size(), expect);
        }
}

BOOST_AUTO_TEST_CASE(geometry_fibonacci_vs_rtree)
{
    for (auto fib : fibonacci_levels | std::views::take(8)) {
        auto rtree = index::rtree<geographic::point, index::rstar<4>>{};
        for (auto i : std::views::iota(0u, fib.num_points))
            rtree.insert(fib[i]);
        auto step = fib.step();
        for (auto p : geographic_random() | std::views::take(60)) {
            auto d = distance(p, fib[fib.nearest(p)]);
            BOOST_CHECK_LE(d, step);
            BOOST_CHECK_LE(d, distance(p, *rtree.qbegin(index::nearest(p, 1))));
        }
    }
}

BOOST_AUTO_TEST_CASE(geometry_raster)
{
    auto ext = cartesian::box{};
    transform(
        geographic::box{{-180., -85.}, {180., 85.}}, ext, srs_forward(tf));
    for (auto p : geographic_random() | std::views::take(10)) {
        auto node = geographic::point{};
        auto step = 0.;
        for (auto res : {2'000, 200, 20, 2}) {
            auto px = cartesian::segment{};
            tf.forward(geographic::segment{p, add_meters(p, 0, res)}, px);
            auto steps = std::unordered_set<double>{};
            for (auto w : {width, width / 2}) {
                auto mat = affine(w, height, px);
                auto in = *transform(mat_forward(mat))(
                    bounds<cartesian::box>(w, height));
                if (!covered_by(in, ext))
                    continue;
                auto grid =
                    geographic_interpolate(w, height, mat, crs, res * 256.);
                BOOST_REQUIRE(!grid.empty());
                auto& lls = grid.begin()->second;
                steps.insert(grid.begin()->first);
                auto footprint = geographic::box{};
                BOOST_REQUIRE(transform(in, footprint, srs_inverse(tf)));
                for (auto const& level : grid)
                    for (auto const& ll : level.second)
                        BOOST_CHECK_LE(distance(ll, footprint), level.first);
                BOOST_CHECK(covered_by(
                    footprint, return_envelope<geographic::box>(lls)));
                if (!step) {
                    node = *std::ranges::min_element(
                        lls, {}, [&](auto const& q) { return distance(p, q); });
                    step = grid.begin()->first;
                    // Shift the center so zooming moves the node off-screen.
                    p = add_meters(node, step * .9, 0.);
                }
                BOOST_CHECK(std::ranges::contains(grid.at(step), node));
            }
            BOOST_CHECK_LE(steps.size(), 1);
        }
    }
}
