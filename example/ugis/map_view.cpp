// Andrew Naplavkov

#include <QMainWindow>
#include <QMouseEvent>
#include <QStatusBar>
#include <QWheelEvent>
#include <boat/geometry/raster.hpp>
#include <boat/gui/qt.hpp>
#include "catalog.h"
#include "map_view.h"

namespace {

namespace geo = boat::geometry;

geo::matrix make_affine(  //
    int width,
    int height,
    geo::geographic::point const& center,
    double scale,
    geo::srs_variant const& crs)
{
    auto fwd = std::visit(
        [&](auto const& v) {
            return geo::transform(geo::srs_forward(geo::transformation(v)));
        },
        crs);
    auto a = *fwd(center);
    auto b = *fwd(geo::add_meters(center, scale, 0.));
    auto px = geo::cartesian::segment{{a.x(), a.y()}, {b.x(), b.y()}};
    return geo::affine(width, height, px);
}

std::optional<geo::geographic::point> pixel_to_lonlat(
    QPointF pixel,
    geo::matrix const& affine,
    geo::srs_variant const& crs)
{
    auto inv = std::visit(
        [&](auto const& v) {
            return geo::transform(  //
                geo::mat_forward(affine),
                geo::srs_inverse(geo::transformation(v)));
        },
        crs);
    return inv(geo::geographic::point{pixel.x(), pixel.y()});
}

}  // namespace

void map_view::set_layers(std::vector<leaf> layers)
{
    layers_ = std::move(layers);
    schedule_redraw();
}

map_view::map_view(QWidget* parent)
    : QWidget(parent), cache_(std::make_shared<boat::gui::caches::lru>(10'000))
{
    setMouseTracking(true);
    setAttribute(Qt::WA_OpaquePaintEvent);
}

void map_view::timerEvent(QTimerEvent* event)
{
    if (event->timerId() != redraw_timer_.timerId()) {
        QWidget::timerEvent(event);
        return;
    }
    redraw_timer_.stop();
    redraw();
}

void map_view::redraw()
{
    tasks_.request_stop();
    auto w = width();
    auto h = height();
    if (w <= 0 || h <= 0)
        return;
    auto crs = geo::ortho(center_);
    auto affine = make_affine(w, h, center_, scale_, crs);
    auto num_points = static_cast<size_t>(
        (w * h) / (boat::tile::size * boat::tile::size) + 4);
    tasks_.run([=, lyrs = layers_](auto tok) {
        auto img = QImage{w, h, QImage::Format_ARGB32_Premultiplied};
        img.fill(Qt::white);
        auto art = QPainter{&img};
        art.setRenderHint(QPainter::Antialiasing);
        art.setCompositionMode(QPainter::CompositionMode_Darken);
        for (auto& l : lyrs) {
            if (tok.stop_requested())
                return;
            try {
                if (!l.layer.raster) {
                    art.setPen(l.pen);
                    art.setBrush(l.brush);
                }
                auto pvd = boat::gui::provider{
                    .catalog = [cat = make_catalog(l.address)]
                    -> decltype(auto) { return *cat; },
                    .layer = l.layer,
                    .cache = cache_,
                    .key = l.cache,
                    .grid = geo::geographic_interpolate(
                        w, h, affine, crs, num_points)};
                auto drw = boat::gui::draw_variant(art, affine, crs);
                for (auto var : pvd.variants()) {
                    if (tok.stop_requested())
                        return;
                    std::visit(drw, var);
                }
            }
            catch (std::exception const& e) {
                qWarning() << "draw error:" << e.what();
            }
        }
        QMetaObject::invokeMethod(
            this,
            [=, img = std::move(img)] mutable {
                if (tok.stop_requested())
                    return;
                img_ = std::move(img);
                update();
            },
            Qt::QueuedConnection);
    });
}

void map_view::paintEvent(QPaintEvent*)
{
    auto art = QPainter{this};
    if (img_.isNull())
        art.fillRect(rect(), Qt::white);
    else
        art.drawImage(0, 0, img_);
}

void map_view::mousePressEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton) {
        panning_ = true;
        last_pos_ = event->pos();
        setCursor(Qt::ClosedHandCursor);
    }
}

void map_view::mouseMoveEvent(QMouseEvent* event)
{
    if (panning_) {
        auto delta = event->pos() - last_pos_;
        last_pos_ = event->pos();
        auto crs = geo::ortho(center_);
        auto affine = make_affine(width(), height(), center_, scale_, crs);
        auto px = QPointF{width() / 2. - delta.x(), height() / 2. - delta.y()};
        if (auto ll = pixel_to_lonlat(px, affine, crs))
            center_ = geo::wrap(*ll);
        schedule_redraw();
    }
    update_status(event->pos());
}

void map_view::mouseReleaseEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton) {
        panning_ = false;
        setCursor(Qt::ArrowCursor);
    }
}

void map_view::wheelEvent(QWheelEvent* event)
{
    auto degrees = event->angleDelta().y() / 8.;
    auto factor = std::pow(2., degrees / 60.);
    auto scale = std::clamp(scale_ / factor, 1., 1e5);
    auto crs = geo::ortho(center_);
    auto cursor = event->position();
    auto a = pixel_to_lonlat(
        cursor, make_affine(width(), height(), center_, scale_, crs), crs);
    auto b = pixel_to_lonlat(
        cursor, make_affine(width(), height(), center_, scale, crs), crs);
    if (a && b)
        center_ = geo::wrap(
            {center_.x() + a->x() - b->x(), center_.y() + a->y() - b->y()});
    scale_ = scale;
    schedule_redraw();
}

void map_view::update_status(QPoint cursor)
{
    auto crs = geo::ortho(center_);
    auto affine = make_affine(width(), height(), center_, scale_, crs);
    auto msg = QString{};
    if (auto ll = pixel_to_lonlat(QPointF{cursor}, affine, crs))
        msg = QString::asprintf("lon: %.6f  lat: %.6f", ll->x(), ll->y());
    if (auto mw = qobject_cast<QMainWindow*>(window()))
        if (auto sb = mw->statusBar())
            sb->showMessage(msg);
}
