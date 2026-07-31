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
#include "task_group.h"
#include "tree.h"

class map_view : public QWidget {
    Q_OBJECT

public:
    explicit map_view(QWidget* parent = nullptr);

public slots:
    void set_layers(std::vector<leaf>);

protected:
    void mouseMoveEvent(QMouseEvent*) override;
    void mousePressEvent(QMouseEvent*) override;
    void mouseReleaseEvent(QMouseEvent*) override;
    void paintEvent(QPaintEvent*) override;
    void resizeEvent(QResizeEvent*) override { schedule_redraw(); }
    void timerEvent(QTimerEvent*) override;
    void wheelEvent(QWheelEvent*) override;

private:
    void redraw();
    void schedule_redraw() { redraw_timer_.start(100, this); }
    void update_status(QPoint cursor);

    std::shared_ptr<boat::gui::caches::lru> cache_;
    boat::geometry::geographic::point center_;
    QImage img_;
    QPoint last_pos_;
    std::vector<leaf> layers_;
    bool panning_ = false;
    QBasicTimer redraw_timer_;
    double scale_ = 1e4;  //< meters per pixel
    task_group tasks_;
};

#endif  // MAP_VIEW_H
