// Andrew Naplavkov

#define BOOST_TEST_MODULE boat_gui_qt

#include <boat/gui/qt.hpp>
#include <boost/test/included/unit_test.hpp>
#include "../context.hpp"
#include "../datasets.hpp"

BOOST_AUTO_TEST_CASE(draw)
{
    auto pixels_sum = 0., transparent_sum = 0.;
    auto dss = datasets() | std::ranges::to<std::vector>();
    for (auto [i, ctx] : std::views::enumerate(contexts())) {
        auto img =
            QImage{ctx.width, ctx.height, QImage::Format_ARGB32_Premultiplied};
        img.fill(Qt::transparent);
        auto paint = QPainter{&img};
        paint.setCompositionMode(QPainter::CompositionMode_Darken);
        paint.setRenderHint(QPainter::Antialiasing);
        paint.setPen(QPen(Qt::darkCyan, 5));
        paint.setBrush(Qt::cyan);
        for (auto& ds : dss) {
            auto lyr = ds->layers().front();
            for (auto feat : ds->features(lyr, ctx.grid, ctx.resolution))
                boat::gui::qt::draw(feat, paint, ctx.affine, ctx.srs);
        }
        auto path = boat::concat(std::setfill('0'), std::setw(2), i, ".png");
        img.save(path.data());
        auto rgb = reinterpret_cast<QRgb*>(img.bits());
        auto num_pixels = ctx.width * ctx.height;
        pixels_sum += num_pixels;
        transparent_sum += std::count_if(
            rgb, rgb + num_pixels, [](auto c) { return !qAlpha(c); });
    }
    BOOST_TEST(transparent_sum / pixels_sum == transparent_ratio,
               boost::test_tools::tolerance(transparent_tolerance));
}
