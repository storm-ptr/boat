// Andrew Naplavkov

#include <QMainWindow>
#include <QMouseEvent>
#include <QStatusBar>
#include <QWheelEvent>
#include <boat/gui/qt.hpp>
#include <boost/gil.hpp>
#include "geometry.h"
#include "map_view.h"

constexpr auto lat_max = 89.;
namespace geo = boat::geometry;
using point = geo::geographic::point;

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
    if (event->timerId() != redraw_timer_.timerId())
        return QWidget::timerEvent(event);
    redraw_timer_.stop();
    redraw();
}

void map_view::resizeEvent(QResizeEvent*)
{
    update();
    schedule_redraw();
}

void map_view::paintEvent(QPaintEvent*)
{
    auto art = QPainter{this};
    art.fillRect(rect(), Qt::white);
    if (img_.isNull())
        return;
    if (boost::geometry::equals(img_mid_, map_mid_) &&
        img_scale_ == map_scale_ && img_.width() == width() &&
        img_.height() == height())
        return art.drawImage(0, 0, img_);
    auto img_crs = geo::ortho(img_mid_);
    auto img_mat =
        affine(img_.width(), img_.height(), img_mid_, img_scale_, img_crs);
    auto map_crs = geo::ortho(map_mid_);
    auto map_mat = affine(width(), height(), map_mid_, map_scale_, map_crs);
    if (img_.format() != QImage::Format_RGBA8888)
        img_ = img_.convertToFormat(QImage::Format_RGBA8888);
    auto bits = reinterpret_cast<boost::gil::rgba8_pixel_t const*>(img_.bits());
    auto gil = boost::gil::interleaved_view(
        img_.width(), img_.height(), bits, img_.bytesPerLine());
    boat::gui::draw_image(
        std::execution::par, gil, img_mat, img_crs, art, map_mat, map_crs);
}

void map_view::mousePressEvent(QMouseEvent* event)
{
    if (event->button() != Qt::LeftButton)
        return;
    panning_pos_ = event->pos();
    setCursor(Qt::ClosedHandCursor);
}

void map_view::mouseMoveEvent(QMouseEvent* event)
{
    auto pos = event->pos();
    auto guard = qScopeGuard([&] { update_status(pos); });
    if (!panning_pos_)
        return;
    auto delta = pos - *std::exchange(panning_pos_, pos);
    auto tf = geo::transformation(geo::ortho(map_mid_));
    auto fwd = geo::transform(geo::srs_forward(tf));
    auto xy = fwd(map_mid_);
    if (!xy)
        return;
    auto mat = affine(width(), height(), map_mid_, map_scale_, fwd);
    auto cursor =
        geo::transform(geo::mat_forward(mat))(point(pos.x(), pos.y()));
    auto pole_y = boat::numbers::earth::equatorial_radius *
                  std::cos(map_mid_.y() * boat::numbers::degree);
    auto flip =
        cursor && std::copysign(1., map_mid_.y()) * cursor->y() > pole_y;
    auto eastward = (flip ? delta.x() : -delta.x()) * map_scale_;
    auto northward = delta.y() * map_scale_;
    auto ll = geo::transform(geo::srs_inverse(tf))(
        point(xy->x() + eastward, xy->y() + northward));
    if (!ll || std::abs(ll->y()) > lat_max)
        return;
    map_mid_ = geo::wrap(*ll);
    update();
    schedule_redraw();
}

void map_view::mouseReleaseEvent(QMouseEvent* event)
{
    if (event->button() != Qt::LeftButton)
        return;
    panning_pos_.reset();
    setCursor(Qt::ArrowCursor);
}

void map_view::wheelEvent(QWheelEvent* event)
{
    auto pos = event->position();
    auto degrees = event->angleDelta().y() / 8.;
    auto factor = std::pow(2., degrees / 60.);
    auto new_scale = std::clamp(map_scale_ / factor, 1., 1e5);
    auto tf = geo::transformation(geo::ortho(map_mid_));
    auto ll = [&](auto scale) {
        auto mat = affine(width(), height(), map_mid_, scale, tf);
        auto inv = geo::transform(geo::mat_forward(mat), geo::srs_inverse(tf));
        return inv(point(pos.x(), pos.y()));
    };
    auto a = ll(map_scale_), b = ll(new_scale);
    if (a && b && std::abs(map_mid_.y() + a->y() - b->y()) <= lat_max)
        map_mid_ = geo::wrap(
            {map_mid_.x() + a->x() - b->x(), map_mid_.y() + a->y() - b->y()});
    map_scale_ = new_scale;
    update();
    schedule_redraw();
    update_status(pos);
}

void map_view::leaveEvent(QEvent*)
{
    if (auto mw = qobject_cast<QMainWindow*>(window()))
        if (auto sb = mw->statusBar())
            sb->clearMessage();
}

void map_view::update_status(QPointF cursor)
{
    auto msg = QString{};
    auto tf = geo::transformation(geo::ortho(map_mid_));
    auto mat = affine(width(), height(), map_mid_, map_scale_, tf);
    auto inv = geo::transform(geo::mat_forward(mat), geo::srs_inverse(tf));
    if (auto ll = inv(point(cursor.x(), cursor.y())))
        msg = QString::asprintf("lon: %.6f  lat: %.6f", ll->x(), ll->y());
    if (auto mw = qobject_cast<QMainWindow*>(window()))
        if (auto sb = mw->statusBar())
            sb->showMessage(msg);
}
