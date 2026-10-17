// Andrew Naplavkov

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QSaveFile>
#include <boat/detail/string.hpp>
#include <boat/detail/uri.hpp>
#include <boat/gui/caches/cache.hpp>
#include <boat/gui/scene_json.hpp>
#include "tree.h"

namespace {

std::unique_ptr<tree> from_source(  //
    boat::gui::source const& in,
    tree* parent)
{
    auto ret = std::make_unique<tree>(
        branch{.source = named_source(in.db),
               .state = in.children.empty() ? branch_state::blank
                                            : branch_state::ready});
    ret->parent = parent;
    for (auto const& child : in.children)
        if (auto l = std::get_if<boat::gui::layer>(&child)) {
            auto ptr = std::make_unique<tree>(
                leaf{.address = in.db.address,
                     .layer = l->db,
                     .pen = QPen{QColor::fromRgba(l->color), l->width},
                     .brush = l->fill ? QBrush{QColor::fromRgba(*l->fill)}
                                      : QBrush{},
                     .state = l->hidden ? Qt::Unchecked : Qt::Checked,
                     .cache = boat::gui::caches::next_key()});
            ptr->parent = ret.get();
            ret->children.push_back(std::move(ptr));
        }
        else
            ret->children.push_back(
                from_source(std::get<boat::gui::source>(child), ret.get()));
    return ret;
}

boat::gui::source to_source(tree const& in)
{
    auto ret = boat::gui::source{std::get<branch>(in.data).source};
    for (auto const& child : in.children)
        if (auto l = std::get_if<leaf>(&child->data))
            ret.children.emplace_back(boat::gui::layer{
                .db = l->layer,
                .color = l->pen.color().rgba(),
                .width = l->pen.widthF(),
                .fill = l->brush.style() == Qt::NoBrush
                            ? std::nullopt
                            : std::optional<uint32_t>{l->brush.color().rgba()},
                .hidden = l->state != Qt::Checked});
        else
            ret.children.emplace_back(to_source(*child));
    return ret;
}

}  // namespace

boat::db::source named_source(boat::db::source source, std::string fallback)
{
    if (!QString::fromStdString(source.source_name).trimmed().isEmpty())
        return source;
    auto file = QFileInfo(QString::fromStdString(source.address));
    if (file.isFile() || file.isDir()) {
        auto name = file.fileName();
        source.source_name =
            (name.isEmpty() ? file.dir().dirName() : name).toStdString();
        return source;
    }
    source.source_name = std::move(fallback);
    try {
        auto uri = boat::uri::parse(source.address);
        if (!uri.host_spec.empty() || !uri.path.empty())
            source.source_name = boat::concat(
                uri.host_spec, uri.path.empty() ? "" : "/", uri.path);
    }
    catch (std::exception const&) {
    }
    return source;
}

std::unique_ptr<tree> from_scene(boat::gui::scene const& in)
{
    auto ret = std::make_unique<tree>(branch{.state = branch_state::ready});
    for (auto const& source : in.sources)
        ret->children.push_back(from_source(source, ret.get()));
    return ret;
}

boat::gui::scene to_scene(tree const& in)
{
    auto ret = boat::gui::scene{};
    for (auto const& source : in.children)
        ret.sources.push_back(to_source(*source));
    return ret;
}

boat::gui::scene read_scene(QString const& path)
{
    auto file = QFile{path};
    boat::check(file.open(QIODevice::ReadOnly), "cannot open scene");
    auto text = file.readAll();
    boat::check(file.error() == QFileDevice::NoError, "cannot read scene");
    return boat::gui::read_scene_json(text.toStdString());
}

bool write_scene(QString const& path, boat::gui::scene const& in)
try {
    auto text = boat::gui::write_scene_json(in);
    auto file = QSaveFile{path};
    return file.open(QIODevice::WriteOnly) &&
           file.write(text.data(), static_cast<qint64>(text.size())) ==
               static_cast<qint64>(text.size()) &&
           file.commit();
}
catch (std::exception const&) {
    return false;
}

QString to_string(tree* ptr)
{
    auto vis = boat::overloaded{
        [](branch const& v) {
            return QString::fromStdString(v.source.source_name);
        },
        [](leaf const& v) {
            return QString::fromStdString(boat::concat(  //
                v.layer.schema_name,
                v.layer.schema_name.empty() ? "" : ".",
                v.layer.table_name,
                ".",
                v.layer.column_name));
        }};
    return std::visit(vis, ptr->data);
}
