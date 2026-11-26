// Andrew Naplavkov

#include <QComboBox>
#include <QFontDatabase>
#include <QGridLayout>
#include <QPlainTextEdit>
#include <QStyle>
#include <QToolButton>
#include <boat/gui/qt.hpp>
#include "feature.h"
#include "map_view.h"
#include "tree.h"

namespace geo = boat::geometry;

static auto magenta(auto style)
{
    style.setColor(QColor{255, 0, 255, style.color().alpha()});
    return style;
}

struct map_view::selection : QWidget {
    using QWidget::QWidget;
    QComboBox objects;
    QPlainTextEdit fields;
    std::vector<feature> hits;
};

void map_view::init_panel()
{
    panel_ = new selection{this};
    auto& objects = panel_->objects;
    panel_->fields.setFont(QFontDatabase::systemFont(QFontDatabase::FixedFont));
    panel_->fields.setReadOnly(true);
    auto close = new QToolButton{panel_};
    close->setIcon(style()->standardIcon(QStyle::SP_TitleBarCloseButton));
    auto layout = new QGridLayout{panel_};
    layout->addWidget(&objects, 0, 0);
    layout->addWidget(close, 0, 1);
    layout->addWidget(&panel_->fields, 1, 0, 1, 2);
    connect(close, &QToolButton::clicked, this, [this] {
        clear_identify();
        panel_->hide();
    });
    connect(&objects, &QComboBox::currentIndexChanged, this, [this](int row) {
        if (row >= 0)
            panel_->fields.setPlainText(panel_->hits[row].attributes);
        update();
    });
    panel_->hide();
}

QWidget* map_view::panel() const
{
    return panel_;
}

void map_view::clear_identify()
{
    query_tasks_.request_stop();
    panel_->objects.clear();
    panel_->hits.clear();
    panel_->fields.clear();
}

void map_view::show_hits(std::vector<feature> hits)
{
    auto& objects = panel_->objects;
    panel_->hits = std::move(hits);
    for (auto& hit : panel_->hits)
        objects.addItem(tr("%1. %2")
                            .arg(objects.count() + 1)
                            .arg(to_string(layers_[hit.layer].layer)));
    if (panel_->hits.empty())
        panel_->fields.setPlainText("no features found");
    panel_->show();
}

void map_view::draw_highlight(QPainter& art) const
{
    auto row = panel_->objects.currentIndex();
    if (row < 0)
        return;
    auto& hit = panel_->hits[row];
    auto [v, ext] = view();
    auto tf = geo::transformation(geo::ortho(v.center));
    auto mat = geo::affine(ext, v.center, v.resolution, tf);
    auto fwd = geo::transform(geo::forward(tf), geo::backward(mat));
    if (auto geom = fwd(hit.geom)) {
        art.setRenderHint(QPainter::Antialiasing);
        auto& lyr = layers_[hit.layer];
        art.setBrush(magenta(lyr.brush));
        art.setPen(magenta(lyr.pen));
        boat::gui::draw_geometry(art)(*geom);
    }
}
