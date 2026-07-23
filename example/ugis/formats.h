// Andrew Naplavkov

#ifndef FORMATS_H
#define FORMATS_H

#include <QString>
#include <optional>
#include <vector>

constexpr auto workspace_filter = "ugis workspace (*.ugis)";

struct copy_format {
    QString filter;
    QString driver;
    QString extension;

    friend auto operator<=>(copy_format const&, copy_format const&) = default;
};

std::optional<copy_format> copy_as_format(bool raster, QString const& filter);
QString copy_as_filter(bool raster);
QString open_filter();
QString ensure_extension(QString path, QString const& extension);

#endif  // FORMATS_H
