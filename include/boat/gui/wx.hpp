// Andrew Naplavkov

#ifndef BOAT_GUI_WX_HPP
#define BOAT_GUI_WX_HPP

#include <wx/graphics.h>
#include <boat/gui/detail/geometry.hpp>
#include <boat/gui/detail/raster.hpp>

namespace boat::gui {
namespace detail {

wxGraphicsPath& add(geometry::curve auto&& in, wxGraphicsPath& out)
{
    out.MoveToPoint(in.at(0).x(), in.at(0).y());
    for (size_t i = 1; i < in.size(); ++i)
        out.AddLineToPoint(in[i].x(), in[i].y());
    return out;
}

}  // namespace detail

inline auto draw_geometry(wxGraphicsContext& out)
{
    return recursive_fn(
        [&](geometry::point auto&& v) { out.DrawEllipse(v.x(), v.y(), 1, 1); },
        [&](geometry::linestring auto&& v) {
            auto path = out.CreatePath();
            out.StrokePath(detail::add(v, path));
        },
        [&](geometry::polygon auto&& v) {
            auto path = out.CreatePath();
            detail::add(v.outer(), path).CloseSubpath();
            for (auto& item : v.inners())
                detail::add(item, path).CloseSubpath();
            out.DrawPath(path);
        });
}

/// Affine matrices map pixels to spatial coordinates.
void draw_image(  //
    execution_policy auto policy,
    boost::gil::rgba8c_view_t in,
    geometry::matrix const& in_affine,
    geometry::srs_spec auto&& in_crs,
    wxGraphicsContext& out,
    geometry::matrix const& out_affine,
    geometry::srs_spec auto&& out_crs)
{
    auto img = wxImage{};
    wxDouble w, h;
    out.GetSize(&w, &h);
    auto rect = draw_image(
        policy,
        in,
        in_affine,
        in_crs,
        out_affine,
        out_crs,
        {0, 0, int(std::ceil(w)), int(std::ceil(h))},
        [&](int w, int h) {
            img = wxImage{w, h};
            img.InitAlpha();
            return [rgb = reinterpret_cast<wxImage::RGBValue*>(img.GetData()),
                    alpha = img.GetAlpha()](auto i, auto px) {
                rgb[i] = {px[0], px[1], px[2]};
                alpha[i] = px[3];
            };
        });
    if (rect) {
        auto [x, y, w, h] = *rect;
        out.DrawBitmap(out.CreateBitmapFromImage(img), x, y, w, h);
    }
}

}  // namespace boat::gui

#endif  // BOAT_GUI_WX_HPP
