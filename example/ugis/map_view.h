// Andrew Naplavkov

#ifndef MAP_VIEW_H
#define MAP_VIEW_H

#include <QBasicTimer>
#include <QImage>
#include <QPoint>
#include <QWidget>
#include <boat/geometry/raster.hpp>
#include <boat/gui/caches/lru.hpp>
#include <boat/gui/provider.hpp>
#include <memory>
#include <optional>
#include "task_group.h"
#include "tree.h"

class map_view : public QWidget {
    Q_OBJECT

public:
    explicit map_view(QWidget* parent = nullptr);

public slots:
    void set_layers(std::vector<leaf>);

protected:
    void leaveEvent(QEvent*) override;
    void mouseMoveEvent(QMouseEvent*) override;
    void mousePressEvent(QMouseEvent*) override;
    void mouseReleaseEvent(QMouseEvent*) override;
    void paintEvent(QPaintEvent*) override;
    void resizeEvent(QResizeEvent*) override;
    void timerEvent(QTimerEvent*) override;
    void wheelEvent(QWheelEvent*) override;

private:
    void redraw();
    void schedule_redraw() { redraw_timer_.start(250, this); }
    void update_status(QPointF cursor);

    std::shared_ptr<boat::gui::caches::lru> cache_;
    QImage img_;
    boat::geometry::geographic::point img_mid_;
    double img_scale_;
    std::vector<leaf> layers_;
    boat::geometry::geographic::point map_mid_;
    double map_scale_ = 1e4;  //< meters per pixel
    std::optional<QPoint> panning_pos_;
    QBasicTimer redraw_timer_;
    task_group tasks_{1};
};

#endif  // MAP_VIEW_H
