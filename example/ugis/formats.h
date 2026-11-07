// Andrew Naplavkov

#ifndef FORMATS_H
#define FORMATS_H

#include <QString>
#include <span>
#include <vector>

constexpr auto scene_filter = "ugis scene (*.ugis)";

struct copy_format {
    QString filter;
    QString driver;
    QString extension;
};

std::vector<copy_format> copy_formats(bool raster);
QString copy_as_filter(std::span<copy_format const> fmts);
QString open_filter();
QString ensure_extension(QString path, QString const& extension);

#endif  // FORMATS_H
