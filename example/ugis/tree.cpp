// Andrew Naplavkov

#include <QFile>
#include <QSaveFile>
#include <boat/detail/string.hpp>
#include <boat/gui/caches/cache.hpp>
#include "tree.h"

namespace {

constexpr auto max_workspace_depth = 128u;
constexpr auto max_workspace_nodes = quint32{100'000};

QDataStream& operator<<(QDataStream& out, node const& in)
{
    auto vis = boat::overloaded{
        [&](branch const& v) {
            out << QString::fromStdString(v.source.source_name)
                << QString::fromStdString(v.source.address)
                << (v.state == branch_state::ready);
        },
        [&](leaf const& v) {
            out << QString::fromStdString(v.address)
                << QString::fromStdString(v.layer.schema_name)
                << QString::fromStdString(v.layer.table_name)
                << QString::fromStdString(v.layer.column_name) << v.layer.raster
                << v.pen << v.brush << (v.state == Qt::Checked);
        }};
    out << static_cast<quint8>(in.index());
    std::visit(vis, in);
    return out;
}

template <class T>
T get(QDataStream& in)
{
    T ret{};
    in >> ret;
    boat::check(in.status() == QDataStream::Ok, "invalid workspace");
    return ret;
}

template <>
node get<node>(QDataStream& in)
{
    switch (get<quint8>(in)) {
        case boat::variant_index<node, branch>():
            return branch{
                .source =
                    boat::db::source{
                        .source_name = get<QString>(in).toStdString(),
                        .address = get<QString>(in).toStdString(),
                    },
                .state =
                    get<bool>(in) ? branch_state::ready : branch_state::blank,
            };
        case boat::variant_index<node, leaf>():
            return leaf{
                .address = get<QString>(in).toStdString(),
                .layer =
                    boat::db::layer{
                        .schema_name = get<QString>(in).toStdString(),
                        .table_name = get<QString>(in).toStdString(),
                        .column_name = get<QString>(in).toStdString(),
                        .raster = get<bool>(in),
                    },
                .pen = get<QPen>(in),
                .brush = get<QBrush>(in),
                .state = get<bool>(in) ? Qt::Checked : Qt::Unchecked,
                .cache = boat::gui::caches::next_key(),
            };
    }
    throw std::runtime_error("invalid node");
}

bool with_file(QIODevice& file, QIODevice::OpenMode mode, auto&& fn)
{
    if (!file.open(mode))
        return false;
    auto io = QDataStream{&file};
    io.setVersion(QDataStream::Qt_DefaultCompiledVersion);
    std::invoke(fn, io);
    return io.status() == QDataStream::Ok;
}

void read_tree(QDataStream& in, tree& out, quint32& remaining, unsigned depth)
{
    boat::check(depth < max_workspace_depth, "workspace too deep");
    out.data = get<node>(in);
    auto count = get<quint32>(in);
    boat::check(count <= remaining, "too many workspace nodes");
    remaining -= count;
    out.children.resize(count);
    for (auto& ch : out.children) {
        ch = std::make_unique<tree>();
        read_tree(in, *ch, remaining, depth + 1);
        ch->parent = &out;
    }
}

QDataStream& operator<<(QDataStream& out, tree const& in)
{
    out << in.data << static_cast<quint32>(in.children.size());
    for (auto& ch : in.children)
        out << *ch;
    return out;
}

}  // namespace

bool read(QString const& path, tree& out)
try {
    auto file = QFile{path};
    return with_file(file, QIODevice::ReadOnly, [&](QDataStream& in) {
        auto remaining = max_workspace_nodes - 1;
        read_tree(in, out, remaining, 0);
    });
}
catch (std::exception const&) {
    return false;
}

bool write(QString const& path, tree const& in)
{
    auto file = QSaveFile{path};
    return with_file(file,
                     QIODevice::WriteOnly,
                     [&](QDataStream& out) { out << in; }) &&
           file.commit();
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
