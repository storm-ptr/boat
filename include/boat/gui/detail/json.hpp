// Andrew Naplavkov

#ifndef BOAT_GUI_JSON_HPP
#define BOAT_GUI_JSON_HPP

#include <boat/gui/scene.hpp>
#include <charconv>
#include <cmath>
#include <format>
#include <nlohmann/json.hpp>

namespace boat::gui {

using json = nlohmann::ordered_json;
constexpr unsigned max_source_depth = 32;

inline json::array_t const& array(json const& obj, char const* key)
{
    static auto const empty = json::array_t{};
    auto it = obj.find(key);
    return it == obj.end() ? empty : it->get_ref<json::array_t const&>();
}

inline uint32_t color(json const& value, char const* field)
{
    auto const& str = value.get_ref<std::string const&>();
    check((str.size() == 7 || str.size() == 9) && str.front() == '#', field);
    uint32_t ret = 0;
    auto [ptr, ec] =
        std::from_chars(str.data() + 1, str.data() + str.size(), ret, 16);
    check(ec == std::errc{} && ptr == str.data() + str.size(), field);
    return str.size() == 7 ? ret | 0xff000000 : ret;
}

inline std::string color(uint32_t value)
{
    return value >> 24 == 255 ? std::format("#{:06X}", value & 0xffffff)
                              : std::format("#{:08X}", value);
}

inline void validate(layer const& v)
{
    check(!v.db.table_name.empty(), "table_name");
    check(!v.db.column_name.empty(), "column_name");
    check(std::isfinite(v.width) && v.width >= 0, "width");
}

inline layer read_layer(json const& obj)
{
    auto ret = layer{{
        .schema_name = obj.value("schema_name", std::string{}),
        .table_name = obj.at("table_name").get<std::string>(),
        .column_name = obj.at("column_name").get<std::string>(),
        .raster = obj.value("raster", false),
    }};
    ret.hidden = obj.value("hidden", false);
    ret.width = obj.value("width", ret.width);
    if (obj.contains("color"))
        ret.color = color(obj["color"], "color");
    if (obj.contains("fill"))
        ret.fill = color(obj["fill"], "fill");
    validate(ret);
    return ret;
}

inline source read_source(json const& obj, unsigned depth = 0)
{
    check(depth < max_source_depth, "source depth");
    auto ret = source{{
        .source_name = obj.value("source_name", std::string{}),
        .address = obj.at("address").get<std::string>(),
    }};
    check(!ret.db.address.empty(), "address");
    for (auto const& child : array(obj, "children"))
        if (child.contains("address"))
            ret.children.emplace_back(read_source(child, depth + 1));
        else
            ret.children.emplace_back(read_layer(child));
    return ret;
}

inline json write_layer(layer const& in)
{
    validate(in);
    auto ret = json{{"table_name", in.db.table_name},
                    {"column_name", in.db.column_name}};
    if (!in.db.schema_name.empty())
        ret["schema_name"] = in.db.schema_name;
    if (in.db.raster)
        ret["raster"] = true;
    if (in.hidden)
        ret["hidden"] = true;
    if (in.color != 0xff000000)
        ret["color"] = color(in.color);
    if (in.width != 1)
        ret["width"] = in.width;
    if (in.fill)
        ret["fill"] = color(*in.fill);
    return ret;
}

inline json write_source(source const& in, unsigned depth = 0)
{
    check(depth < max_source_depth, "source depth");
    check(!in.db.address.empty(), "address");
    auto ret = json{{"address", in.db.address}};
    if (!in.db.source_name.empty())
        ret["source_name"] = in.db.source_name;
    for (auto const& child : in.children)
        if (auto l = std::get_if<layer>(&child))
            ret["children"].push_back(write_layer(*l));
        else
            ret["children"].push_back(
                write_source(std::get<source>(child), depth + 1));
    return ret;
}

}  // namespace boat::gui

#endif  // BOAT_GUI_JSON_HPP
