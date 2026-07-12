// Andrew Naplavkov

#include <QDebug>
#include <QFile>
#include <boat/detail/string.hpp>
#include <boat/gui/caches/cache.hpp>
#include "tree.h"

namespace {

QDataStream& operator<<(QDataStream& out, node const& in)
{
    out << static_cast<quint8>(in.index());
    std::visit(  //
        boat::overloaded{
            [&](branch const& v) {
                out << QString::fromStdString(v.source.source_name)
                    << QString::fromStdString(v.source.address)
                    << (v.state == branch_state::ready);
            },
            [&](leaf const& v) {
                out << QString::fromStdString(v.address)
                    << QString::fromStdString(v.layer.schema_name)
                    << QString::fromStdString(v.layer.table_name)
                    << QString::fromStdString(v.layer.column_name)
                    << v.layer.raster << v.pen << v.brush
                    << (v.state == Qt::Checked);
            },
        },
        in);
    return out;
}

template <class T>
T get(QDataStream& in)
{
    T ret;
    in >> ret;
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

bool with_file(QString const& path, QIODevice::OpenMode mode, auto&& fn)
{
    auto file = QFile{path};
    if (!file.open(mode))
        return false;
    auto io = QDataStream{&file};
    io.setVersion(QDataStream::Qt_DefaultCompiledVersion);
    std::invoke(fn, io);
    return io.status() == QDataStream::Ok;
}

QDataStream& operator>>(QDataStream& in, tree& out)
{
    out.data = get<node>(in);
    out.children.resize(get<quint32>(in));
    for (auto& ch : out.children) {
        ch = std::make_unique<tree>();
        in >> *ch;
        ch->parent = &out;
    }
    return in;
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
{
    return with_file(
        path, QIODevice::ReadOnly, [&](QDataStream& in) { in >> out; });
}

bool write(QString const& path, tree const& in)
{
    return with_file(
        path, QIODevice::WriteOnly, [&](QDataStream& out) { out << in; });
}

QString to_string(tree* ptr)
{
    return std::visit(
        boat::overloaded{
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
            },
        },
        ptr->data);
}
