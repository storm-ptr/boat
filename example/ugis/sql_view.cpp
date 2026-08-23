// Andrew Naplavkov

#include <QFontDatabase>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QSplitter>
#include <QVBoxLayout>
#include <boat/db/io.hpp>
#include <boat/gdal/command.hpp>
#include <boat/gdal/dataset.hpp>
#include <boat/sql/commands.hpp>
#include "sql_view.h"

namespace {

std::unique_ptr<boat::db::command> make_command(std::string const& address)
{
    if (boat::sql::supported_url(address))
        return boat::sql::make_command(address);
    auto ret = std::make_unique<boat::gdal::command>();
    ret->dataset = boat::gdal::open(address.data());
    ret->dialect = "OGRSQL";
    return ret;
}

QString format(boat::db::rowset const& rows)
{
    if (rows.columns.empty())
        return "completed";
    constexpr auto limit = size_t{100};
    auto shown = rows;
    if (shown.rows.size() > limit)
        shown.rows.resize(limit);
    auto ret = QString::fromStdString(boat::concat(shown));
    ret += shown.rows.size() == rows.rows.size()
               ? QString("%1 rows").arg(rows.rows.size())
               : QString("%1 of %2 rows shown")
                     .arg(shown.rows.size())
                     .arg(rows.rows.size());
    return ret;
}

}  // namespace

sql_view::sql_view(std::string address, QWidget* parent)
    : QWidget(parent), address_(std::move(address))
{
    run_ = new QPushButton{"run", this};
    cancel_ = new QPushButton{"cancel", this};
    cancel_->setEnabled(false);
    auto close = new QPushButton{"close", this};
    auto show_address = new QPushButton{"address", this};
    show_address->setCheckable(true);
    for (auto button : {run_, cancel_, close, show_address})
        button->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    auto address_field = new QLineEdit{QString::fromStdString(address_), this};
    address_field->setReadOnly(true);
    auto policy = address_field->sizePolicy();
    policy.setRetainSizeWhenHidden(true);
    address_field->setSizePolicy(policy);
    address_field->hide();
    auto bar = new QHBoxLayout{};
    bar->addWidget(run_);
    bar->addWidget(cancel_);
    bar->addWidget(close);
    bar->addWidget(show_address);
    bar->addWidget(address_field, 1);

    auto font = QFontDatabase::systemFont(QFontDatabase::FixedFont);
    query_ = new QPlainTextEdit{this};
    query_->setFont(font);
    output_ = new QPlainTextEdit{this};
    output_->setFont(font);
    output_->setReadOnly(true);
    auto split = new QSplitter{Qt::Vertical, this};
    split->addWidget(query_);
    split->addWidget(output_);

    auto layout = new QVBoxLayout{this};
    layout->addLayout(bar);
    layout->addWidget(split, 1);
    connect(run_, &QPushButton::clicked, this, [this] { run(); });
    connect(cancel_, &QPushButton::clicked, this, [this] {
        tasks_.request_stop();
    });
    connect(close, &QPushButton::clicked, this, [this] {
        if (close_handler_)
            close_handler_();
    });
    connect(show_address,
            &QPushButton::toggled,
            address_field,
            &QWidget::setVisible);
}

void sql_view::close_async()
{
    closing_ = true;
    hide();
    tasks_.request_stop();
    if (!tasks_.busy())
        deleteLater();
}

void sql_view::run()
{
    auto sql = query_->toPlainText().toStdString();
    if (sql.empty())
        return;
    tasks_.request_stop();
    cancel_->setEnabled(true);
    output_->clear();
    auto fut = tasks_.run([this, sql = std::move(sql)](auto tok) {
        if (tok.stop_requested())
            return;
        try {
            auto rows = make_command(address_)->exec(sql);
            if (tok.stop_requested())
                return;
            auto text = format(rows);
            QMetaObject::invokeMethod(
                this,
                [this, tok, text = std::move(text)] {
                    if (!tok.stop_requested())
                        output_->setPlainText(text);
                },
                Qt::QueuedConnection);
        }
        catch (std::exception const& e) {
            auto text = QString::fromUtf8(e.what());
            QMetaObject::invokeMethod(
                this,
                [this, tok, text = std::move(text)] {
                    if (!tok.stop_requested())
                        output_->setPlainText("error: " + text);
                },
                Qt::QueuedConnection);
        }
    });
    watch_task(std::move(fut));
}

void sql_view::set_close_handler(std::function<void()> fn)
{
    close_handler_ = std::move(fn);
}

void sql_view::watch_task(QFuture<void> fut)
{
    fut.then(this, [this] {
        auto busy = tasks_.busy();
        cancel_->setEnabled(busy);
        if (closing_ && !busy)
            deleteLater();
    });
}
