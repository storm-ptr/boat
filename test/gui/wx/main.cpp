// Andrew Naplavkov

#define BOOST_TEST_MODULE boat_gui_wx

#include <boat/gui/wx.hpp>
#include <boost/test/included/unit_test.hpp>
#include "../context.hpp"
#include "../datasets.hpp"

namespace {

auto make_graphics_context(auto& v)
{
    auto ret = std::unique_ptr<wxGraphicsContext>{wxGraphicsContext::Create(v)};
    ret->SetPen(wxPen{wxColour{0, 139, 139}, 5});
    ret->SetBrush(*wxCYAN_BRUSH);
    return ret;
}

void compose_darken(wxImage& lhs, wxImage const& rhs)
{
    auto ys = std::views::iota(0, lhs.GetHeight());
    std::for_each(std::execution::par, ys.begin(), ys.end(), [&](int y) {
        auto d = y * lhs.GetWidth();
        auto lhs_alpha = lhs.GetAlpha() + d;
        auto rhs_alpha = rhs.GetAlpha() + d;
        auto lhs_rgb = reinterpret_cast<wxImage::RGBValue*>(lhs.GetData()) + d;
        auto rhs_rgb = reinterpret_cast<wxImage::RGBValue*>(rhs.GetData()) + d;
        for (int x{}; x < lhs.GetWidth(); ++x) {
            lhs_alpha[x] = std::max<>(lhs_alpha[x], rhs_alpha[x]);
            if (rhs_alpha[x] == wxIMAGE_ALPHA_TRANSPARENT)
                continue;
            lhs_rgb[x].red = std::min<>(lhs_rgb[x].red, rhs_rgb[x].red);
            lhs_rgb[x].green = std::min<>(lhs_rgb[x].green, rhs_rgb[x].green);
            lhs_rgb[x].blue = std::min<>(lhs_rgb[x].blue, rhs_rgb[x].blue);
        }
    });
}

}  // namespace

BOOST_AUTO_TEST_CASE(draw)
{
    wxInitAllImageHandlers();
    auto pixels_sum = 0., transparent_sum = 0.;
    auto dss = datasets() | std::ranges::to<std::vector>();
    for (auto [i, ctx] : std::views::enumerate(contexts())) {
        auto img = wxImage(ctx.width, ctx.height);
        img.InitAlpha();
        auto num_pixels = ctx.width * ctx.height;
        std::memset(img.GetAlpha(), wxIMAGE_ALPHA_TRANSPARENT, num_pixels);
        std::memset(img.GetData(), UCHAR_MAX, 3 * num_pixels);
        for (auto& ds : dss) {
            auto tmp = img.Copy();
            auto gc = make_graphics_context(tmp);
            auto lyr = ds->layers().front();
            for (auto feat : ds->features(lyr, ctx.grid, ctx.resolution))
                boat::gui::wx::draw(feat, *gc, ctx.affine, ctx.srs);
            gc.reset();
            compose_darken(img, tmp);
        }
        auto path = boat::concat(std::setfill('0'), std::setw(2), i, ".png");
        img.SaveFile(wxASCII_STR(path.data()), wxBITMAP_TYPE_PNG);
        pixels_sum += num_pixels;
        transparent_sum += std::count(img.GetAlpha(),
                                      img.GetAlpha() + num_pixels,
                                      wxIMAGE_ALPHA_TRANSPARENT);
    }
    BOOST_TEST(transparent_sum / pixels_sum == transparent_ratio,
               boost::test_tools::tolerance(transparent_tolerance));
}
