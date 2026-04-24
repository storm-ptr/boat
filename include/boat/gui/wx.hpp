// Andrew Naplavkov

#ifndef BOAT_GUI_WX_HPP
#define BOAT_GUI_WX_HPP

#include <wx/graphics.h>
#include <wx/mstream.h>
#include <wx/wx.h>
#include <boat/gui/detail/geometry.hpp>
#include <boat/gui/variant.hpp>
#include <execution>

namespace boat::gui {

struct wx {
    using image = wxImage;

    static wxGraphicsPath& add(geometry::curve auto&& in, wxGraphicsPath& out)
    {
        out.MoveToPoint(in.at(0).x(), in.at(0).y());
        for (size_t i = 1; i < in.size(); ++i)
            out.AddLineToPoint(in[i].x(), in[i].y());
        return out;
    }

    static auto drawGeometry(wxGraphicsContext& out)
    {
        return overloaded{
            [&](geometry::point auto&& g) {
                out.DrawEllipse(g.x(), g.y(), 1, 1);
            },
            [&](geometry::linestring auto&& g) {
                auto path = out.CreatePath();
                out.StrokePath(add(g, path));
            },
            [&](geometry::polygon auto&& g) {
                auto path = out.CreatePath();
                add(g.outer(), path).CloseSubpath();
                for (auto& item : g.inners())
                    add(item, path).CloseSubpath();
                out.DrawPath(path);
            },
            [](this auto&& self, geometry::multi auto&& g) -> void {
                std::ranges::for_each(g, self);
            },
            [](this auto&& self, geometry::dynamic auto&& g) -> void {
                std::visit(self, g);
            }};
    }

    static bool loadImage(blob_view in, wxImage& out)
    {
        auto log = wxLogNull{};
        auto is = wxMemoryInputStream{in.data(), in.size()};
        return out.LoadFile(is);
    }

    static void drawImage(  //
        wxImage const& in,
        geometry::matrix const& in_affine,
        geometry::srs_params auto&& in_crs,
        wxGraphicsContext& out,
        geometry::matrix const& out_affine,
        geometry::srs_params auto&& out_crs)
    {
        auto [fwd, inv] = bidirectional(in_affine, in_crs, out_affine, out_crs);
        auto mbr1 = wxRect{in.GetSize()};
        auto intersect = [&](geometry::box auto&& g) {
            wxDouble w, h;
            out.GetSize(&w, &h);
            w = std::ceil(w), h = std::ceil(h);
            auto a = g.min_corner(), b = g.max_corner();
            auto x = std::floor(a.x()), y = std::floor(a.y());
            return wxRect2DDouble{0., 0., w, h}.CreateIntersection(
                {x, y, std::ceil(b.x()) - x, std::ceil(b.y()) - y});
        };
        auto mbr2 = fwd(multi_point(mbr1.width, mbr1.height))
                        .transform(geometry::minmax)
                        .transform(intersect);
        if (!mbr2 || mbr2->IsEmpty())
            return;
        auto img = wxImage{mbr2->GetSize()};
        img.InitAlpha();
        auto rgb1 = reinterpret_cast<wxImage::RGBValue*>(in.GetData());
        auto ys = std::views::iota(0, img.GetHeight());
        std::for_each(std::execution::par, ys.begin(), ys.end(), [&](int y) {
            int d = y * img.GetWidth(), dx = mbr2->m_x, dy = mbr2->m_y;
            auto rgb2 = reinterpret_cast<wxImage::RGBValue*>(img.GetData()) + d;
            auto alpha2 = img.GetAlpha() + d;
            for (int x{}; x < img.GetWidth(); ++x) {
                auto p = inv(geometry::geographic::point(x + dx, y + dy))
                             .transform([](geometry::point auto&& p) {
                                 return wxPoint(p.x(), p.y());
                             });
                if (p && mbr1.Contains(*p))
                    rgb2[x] = rgb1[p->y * in.GetWidth() + p->x];
                else
                    alpha2[x] = wxIMAGE_ALPHA_TRANSPARENT;
            }
        });
        out.DrawBitmap(out.CreateBitmapFromImage(img), *mbr2);
    }
};

template <>
struct traits<wxGraphicsContext> {
    using type = wx;
};

}  // namespace boat::gui

#endif  // BOAT_GUI_WX_HPP
