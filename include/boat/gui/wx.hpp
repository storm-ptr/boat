// Andrew Naplavkov

#ifndef BOAT_GUI_WX_HPP
#define BOAT_GUI_WX_HPP

#include <wx/graphics.h>
#include <wx/mstream.h>
#include <wx/wx.h>
#include <boat/gui/detail/geometry.hpp>
#include <boat/gui/feature.hpp>
#include <execution>

namespace boat::gui::wx {
namespace detail {

wxGraphicsPath& insert(wxGraphicsPath& out, geometry::curve auto const& in)
{
    out.MoveToPoint(in.at(0).x(), in.at(0).y());
    for (size_t i = 1; i < in.size(); ++i)
        out.AddLineToPoint(in[i].x(), in[i].y());
    return out;
}

void draw(shape const& feat,
          wxGraphicsContext& art,
          geometry::matrix const& affine,
          geometry::srs_spec auto const& srs)
{
    auto var = forward(epsg(feat.epsg), affine, srs)(variant(feat.wkb));
    if (!var)
        return;
    overloaded{
        [&](geometry::point auto& g) { art.DrawEllipse(g.x(), g.y(), 1, 1); },
        [&](geometry::linestring auto& g) {
            auto path = art.CreatePath();
            art.StrokePath(insert(path, g));
        },
        [&](geometry::polygon auto& g) {
            auto path = art.CreatePath();
            insert(path, g.outer()).CloseSubpath();
            for (auto& item : g.inners())
                insert(path, item).CloseSubpath();
            art.DrawPath(path);
        },
        [](this auto&& self, geometry::multi auto& g) {
            std::ranges::for_each(g, self);
        },
        [](this auto&& self, geometry::dynamic auto& g) {
            std::visit(self, g);
        }}(*var);
}

void draw(raster const& feat,
          wxGraphicsContext& art,
          geometry::matrix const& affine,
          geometry::srs_spec auto const& srs)
{
    auto img1 = wxImage{};
    auto is = wxMemoryInputStream{feat.image.data(), feat.image.size()};
    if (auto _ = wxLogNull{}; !img1.LoadFile(is))
        return;
    auto [fwd, inv] = bidirectional(feat.affine, epsg(feat.epsg), affine, srs);
    auto mbr1 = wxRect{img1.GetSize()};
    auto mbr2 =
        fwd(multi_point(mbr1.width, mbr1.height))
            .transform(geometry::minmax)
            .transform([&](auto const& g) {
                wxDouble width, height;
                art.GetSize(&width, &height);
                width = std::ceil(width), height = std::ceil(height);
                auto a = g.min_corner(), b = g.max_corner();
                auto x = std::floor(a.x()), y = std::floor(a.y());
                return wxRect2DDouble{0., 0., width, height}.CreateIntersection(
                    {x, y, std::ceil(b.x()) - x, std::ceil(b.y()) - y});
            });
    if (!mbr2 || mbr2->IsEmpty())
        return;
    auto img2 = wxImage{mbr2->GetSize()};
    img2.InitAlpha();
    auto rgb1 = reinterpret_cast<wxImage::RGBValue*>(img1.GetData());
    auto ys = std::views::iota(0, img2.GetHeight());
    std::for_each(std::execution::par, ys.begin(), ys.end(), [&](int y) {
        int d = y * img2.GetWidth(), dx = mbr2->m_x, dy = mbr2->m_y;
        auto rgb2 = reinterpret_cast<wxImage::RGBValue*>(img2.GetData()) + d;
        auto alpha2 = img2.GetAlpha() + d;
        for (int x{}; x < img2.GetWidth(); ++x) {
            auto p =
                inv(geometry::geographic::point(x + dx, y + dy))
                    .transform([](auto&& p) { return wxPoint(p.x(), p.y()); });
            if (p && mbr1.Contains(*p))
                rgb2[x] = rgb1[p->y * img1.GetWidth() + p->x];
            else
                alpha2[x] = wxIMAGE_ALPHA_TRANSPARENT;
        }
    });
    art.DrawBitmap(art.CreateBitmapFromImage(img2), *mbr2);
}

}  // namespace detail

void draw(feature const& feat,
          wxGraphicsContext& art,
          geometry::matrix const& affine,
          geometry::srs_spec auto const& srs)
{
    std::visit([&](auto const& v) { detail::draw(v, art, affine, srs); }, feat);
}

}  // namespace boat::gui::wx

#endif  // BOAT_GUI_WX_HPP
