// Andrew Naplavkov

#include <QFontDatabase>
#include <QMetaObject>
#include <QPlainTextEdit>
#include <QPointer>
#include <QSplitter>
#include <QTabWidget>
#include <numbers>
#include "main_window.h"
#include "tree_view.h"

namespace {

QtMessageHandler old_handler;
QPointer<QPlainTextEdit> log_ptr;

void message_handler(  //
    QtMsgType type,
    QMessageLogContext const& ctx,
    QString const& msg)
{
    if (!log_ptr)
        return;
    auto fmt = qFormatLogMessage(type, ctx, msg);
    QMetaObject::invokeMethod(
        log_ptr,
        [fmt] {
            if (log_ptr)
                log_ptr->appendPlainText(fmt);
        },
        Qt::QueuedConnection);
}

}  // namespace

main_window::main_window()
{
    setWindowTitle("ugis");
    auto height = 600;
    auto width = qRound(height * std::numbers::phi);
    resize(width, height);

    auto tree = new tree_view{this};
    log_ = new QPlainTextEdit{this};
    log_->setFont(QFontDatabase::systemFont(QFontDatabase::FixedFont));
    log_->setReadOnly(true);
    log_->setMaximumBlockCount(10'000);

    auto tabs = new QTabWidget{this};
    tabs->setTabPosition(QTabWidget::East);
    tabs->addTab(log_, "log");

    auto splitter = new QSplitter{Qt::Horizontal, this};
    splitter->addWidget(tree);
    splitter->addWidget(tabs);
    auto tree_width = qRound(width / (std::numbers::phi + 1));
    splitter->setSizes({tree_width, width - tree_width});
    setCentralWidget(splitter);

    qSetMessagePattern("[%{time hh:mm:ss}] %{type}: %{message}");
    log_ptr = log_;
    old_handler = qInstallMessageHandler(message_handler);
}

main_window::~main_window()
{
    qInstallMessageHandler(old_handler);
    log_ptr.clear();
}
