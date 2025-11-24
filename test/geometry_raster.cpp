// Andrew Naplavkov

#include <boat/geometry/raster.hpp>
#include <boat/geometry/transform.hpp>
#include <boost/test/unit_test.hpp>

using namespace boat::geometry;
using namespace boost::geometry;

BOOST_AUTO_TEST_CASE(geometry_raster)
{
    auto width = 1920;
    auto height = 1080;
    auto wnd = cartesian::box{{}, {width * 1., height * 1.}};
    auto num_points = 100u;
    auto srs = srs::epsg{3857};
    auto pj = srs::projection<>{srs};
    auto ext = cartesian::box{};
    transform(
        geographic::box{{-180., -85.}, {180., 85.}}, ext, srs_forward(pj));
    for (auto p : random() | std::views::take(10)) {
        for (auto res : {1, 10, 100, 1000}) {
            auto pixel = cartesian::segment{};
            pj.forward(geographic::segment{p, add_meters(p, 0, res)}, pixel);
            auto mat = to_matrix(width, height, pixel);
            auto in = *transform(mat_forward(mat))(wnd);
            if (!covered_by(in, ext))
                continue;

            auto grid = tessellation(width, height, num_points, mat, srs);
            BOOST_CHECK(!grid.empty());
            auto& lls = grid.begin()->second;
            BOOST_CHECK_LE(lls.size(), num_points);
            BOOST_CHECK_GE(lls.size() + 1, num_points / 2);
            auto xys = cartesian::multi_point{};
            BOOST_CHECK(pj.forward(lls, xys));
            auto out = minmax(xys);

            BOOST_CHECK(covered_by(out, in));
            auto iou = area(out) / area(in);
            BOOST_TEST(iou > .88, iou << "; " << res << "m/px; " << wkt(p));
        }
    }
}
