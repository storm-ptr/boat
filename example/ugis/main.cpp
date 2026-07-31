// Andrew Naplavkov

#include <QApplication>
#include <QDir>
#include <QFileInfo>
#include "main_window.h"

int main(int argc, char* argv[])
{
    auto app = QApplication{argc, argv};
    auto wnd = main_window{};
    wnd.show();
    return app.exec();
}