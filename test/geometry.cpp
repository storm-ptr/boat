// Andrew Naplavkov

#include <boat/geometry/transform.hpp>
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

template <ogc99 Geom1, same_tag<Geom1> Geom2>
void check(std::string const& wkt)
{
    auto pj = srs::projection<srs::static_epsg<3857>>{};
    auto ll = from_wkt<Geom1>(wkt);
    auto xy = Geom2{};
    BOOST_CHECK(transform(ll, xy, srs_forward(pj)));
    BOOST_CHECK_NE(wkt, to_wkt(xy));
    auto wkb = boat::blob{} << xy;
    boat::blob_view{wkb} >> xy;
    BOOST_CHECK(transform(xy, ll, srs_inverse(pj)));
    BOOST_CHECK_EQUAL(wkt, to_wkt(ll));
}

}  // namespace

BOOST_AUTO_TEST_CASE(geometry_transform_wkb)
{
    auto geoms = boost::fusion::make_map<geographic::point,
                                         geographic::linestring,
                                         geographic::polygon,
                                         geographic::multi_point,
                                         geographic::multi_linestring,
                                         geographic::multi_polygon,
                                         geographic::geometry_collection>(
        // clang-format off
"POINT(30 10)"s,
"LINESTRING(30 10,10 30,40 40)"s,
"POLYGON((35 10,45 45,15 40,10 20,35 10),(20 30,35 35,30 20,20 30))"s,
"MULTIPOINT((10 40),(40 30),(20 20),(30 10))"s,
"MULTILINESTRING((10 10,20 20,10 40),(40 40,30 30,40 20,30 10))"s,
"MULTIPOLYGON(((40 40,20 45,45 30,40 40)),((20 35,10 30,10 10,30 5,45 20,20 35),(30 20,20 15,20 25,30 20)))"s,
"GEOMETRYCOLLECTION(POINT(40 10),LINESTRING(10 10,20 20,10 40),POLYGON((40 40,20 45,45 30,40 40)))"s
        // clang-format on
    );
    boost::fusion::for_each(geoms, [&]<class P>(P const& pair) {
        using geom1 = P::first_type;
        using geom2 = std::variant_alternative_t<variant_index_v<geom1>,
                                                 cartesian::variant>;
        auto const& wkt = pair.second;
        check<geom1, geom2>(wkt);
        check<geographic::variant, cartesian::variant>(wkt);
    });
}

BOOST_AUTO_TEST_CASE(geometry_endian)
{
    std::pair<std::string, std::string> tests[] = {
        {"000000000140000000000000004010000000000000", "POINT(2 4)"},
        {"010100000000000000000000400000000000001040", "POINT(2 4)"},
    };
    for (auto& [hex, wkt] : tests) {
        auto unhex = boost::algorithm::unhex(hex);
        auto in = boat::blob_view{std::as_bytes(std::span(unhex))};
        auto geom = get<cartesian::variant>(in);
        BOOST_CHECK_EQUAL(wkt, to_wkt(geom));
    }
}
