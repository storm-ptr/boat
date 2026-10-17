// Andrew Naplavkov

#include <boat/gui/scene_json.hpp>
#include <boost/test/unit_test.hpp>
#include <limits>

namespace gui = boat::gui;

BOOST_AUTO_TEST_CASE(scene_defaults)
{
    auto scene = gui::read_scene_json("{}");
    BOOST_TEST(scene.sources.empty());
    BOOST_TEST(scene.view.center.x() == 0);
    BOOST_TEST(scene.view.center.y() == 0);
    BOOST_TEST(scene.view.resolution == 9783.94);
    BOOST_TEST(nlohmann::json::parse(gui::write_scene_json(scene)).empty());
    scene = gui::read_scene_json(R"({"sources":[{"address":"data.gpkg"}]})");
    BOOST_REQUIRE(scene.sources.size() == 1);
    BOOST_TEST(scene.sources[0].children.empty());
    BOOST_TEST(scene.sources[0].db.source_name.empty());
    BOOST_TEST(scene.sources[0].db.address == "data.gpkg");
    auto empty = gui::read_scene_json(
        R"({"sources":[{"address":"data.gpkg","children":[]}]})");
    BOOST_TEST(gui::write_scene_json(scene) == gui::write_scene_json(empty));
}

BOOST_AUTO_TEST_CASE(scene_roundtrip)
{
    auto scene = gui::read_scene_json(R"({
      "center": [12.4964, 41.9028], "resolution": 42.5,
      "sources": [{"address": "data.gpkg", "children": [
        {"table_name":"roads", "column_name":"geom"},
        {"address":"nested.tif", "children":[
          {"table_name":"_layer", "column_name":"raster", "raster":true}
        ]},
        {"schema_name":"public", "table_name":"land", "column_name":"geom",
         "hidden":true, "color":"#80123456", "width":2.5, "fill":"#ABCDEF"}
      ]}]
    })");
    auto& children = scene.sources[0].children;
    BOOST_REQUIRE(children.size() == 3);
    auto& roads = std::get<gui::layer>(children[0]);
    BOOST_TEST(roads.db.schema_name.empty());
    BOOST_TEST(!roads.db.raster);
    BOOST_TEST(!roads.hidden);
    BOOST_TEST(roads.color == 0xff000000);
    BOOST_TEST(roads.width == 1);
    BOOST_TEST(!roads.fill.has_value());
    auto& nested = std::get<gui::source>(children[1]);
    BOOST_TEST(std::get<gui::layer>(nested.children[0]).db.raster);
    auto& land = std::get<gui::layer>(children[2]);
    BOOST_TEST(land.color == 0x80123456);
    BOOST_TEST(*land.fill == 0xffabcdef);
    auto text = gui::write_scene_json(scene);
    auto saved_roads = nlohmann::json::parse(text)["sources"][0]["children"][0];
    BOOST_TEST(saved_roads.size() == 2);
    auto restored = gui::read_scene_json(text);
    BOOST_TEST(restored.view.center.x() == 12.4964);
    BOOST_TEST(restored.view.center.y() == 41.9028);
    BOOST_TEST(restored.view.resolution == 42.5);
    BOOST_TEST(gui::write_scene_json(restored) == text);
}

BOOST_AUTO_TEST_CASE(scene_addresses)
{
    auto scene = gui::read_scene_json(R"({"sources":[
      {"address":"../data/file.gpkg"},
      {"address":"postgres://user:password@localhost/db?sslmode=require"},
      {"address":"PG:dbname='test' user='user' password='secret'"},
      {"address":"/vsicurl/https://example.com/file.tif"},
      {"address":"DSN=test;UID=user;PWD=secret"}
    ]})");
    BOOST_TEST(scene.sources[0].db.address == "../data/file.gpkg");
    BOOST_TEST(scene.sources[1].db.address ==
               "postgres://user:password@localhost/db?sslmode=require");
    BOOST_TEST(scene.sources[2].db.address ==
               "PG:dbname='test' user='user' password='secret'");
    BOOST_TEST(scene.sources[3].db.address ==
               "/vsicurl/https://example.com/file.tif");
    BOOST_TEST(scene.sources[4].db.address ==
               "DSN=test;UID=user;PWD=secret");
    auto restored = gui::read_scene_json(gui::write_scene_json(scene));
    for (size_t i = 0; i < scene.sources.size(); ++i)
        BOOST_TEST(restored.sources[i].db.address ==
                   scene.sources[i].db.address);
}

BOOST_AUTO_TEST_CASE(scene_invalid)
{
    for (auto text : {
             "",
             "{",
             "{} garbage",
             "{} {}",
             "{}]",
             "{\"sources\":[],}",
             "{}/*comment*/",
             "[]",
             "null",
             R"({"sources":null})",
             R"({"sources":{}})",
             R"({"sources":[null]})",
             R"({"sources":[[]]})",
             R"({"sources":[{}]})",
             R"({"sources":[{"address":""}]})",
             R"({"sources":[{"address":42}]})",
             R"({"sources":[{"address":"a","children":null}]})",
             R"({"sources":[{"address":"a","children":{}}]})",
             R"({"sources":[{"address":"a","children":[0]}]})",
             R"({"sources":[{"address":"a","source_name":null}]})",
             R"({"center":[0]})",
             R"({"center":["0",0]})",
             R"({"center":[true,0]})",
             R"({"center":[null,0]})",
             R"({"center":[1e999,0]})",
             R"({"center":[0,1e999]})",
             R"({"resolution":0})",
             R"({"resolution":-1})",
             R"({"resolution":"1"})",
             R"({"resolution":true})",
             R"({"resolution":null})",
             R"({"resolution":1e999})",
         }) {
        BOOST_TEST_CONTEXT(text)
        {
            BOOST_CHECK_THROW(gui::read_scene_json(text), std::exception);
        }
    }
    for (auto fields : {
             R"("hidden":null)",
             R"("hidden":1)",
             R"("raster":"false")",
             R"("schema_name":false)",
             R"("color":"red")",
             R"("color":"#12345G")",
             R"("fill":null)",
             R"("fill":"#123")",
             R"("width":-1)",
             R"("width":"1")",
             R"("width":true)",
             R"("width":null)",
             R"("table_name":"")",
             R"("column_name":"")",
             R"("width":1e999)",
         }) {
        auto text = std::string{R"({"sources":[{"address":"a","children":[{
            "table_name":"t","column_name":"g",)"} +
                    fields + "}]}]}";
        BOOST_TEST_CONTEXT(text)
        {
            BOOST_CHECK_THROW(gui::read_scene_json(text), std::exception);
        }
    }
    BOOST_CHECK_THROW(gui::read_scene_json(R"({"sources":[{"address":"a",
        "children":[{"table_name":"t"}]}]})"),
                      std::exception);
}

BOOST_AUTO_TEST_CASE(scene_invalid_output)
{
    auto scene = gui::read_scene_json(R"({"sources":[{"address":"data.gpkg",
        "children":[{"table_name":"roads","column_name":"geom"}]}]})");
    auto& layer = std::get<gui::layer>(scene.sources[0].children[0]);
    for (double width : {-1.,
                         std::numeric_limits<double>::infinity(),
                         std::numeric_limits<double>::quiet_NaN()}) {
        layer.width = width;
        BOOST_CHECK_THROW(gui::write_scene_json(scene), std::runtime_error);
    }
    layer.width = 1;
    for (double resolution : {0., -1.,
                              std::numeric_limits<double>::infinity(),
                              std::numeric_limits<double>::quiet_NaN()}) {
        scene.view.resolution = resolution;
        BOOST_CHECK_THROW(gui::write_scene_json(scene), std::runtime_error);
    }
    scene.view.resolution = gui::view{}.resolution;
    for (auto name : {&layer.db.table_name,
                      &layer.db.column_name,
                      &scene.sources[0].db.address}) {
        auto saved = std::exchange(*name, "");
        BOOST_CHECK_THROW(gui::write_scene_json(scene), std::runtime_error);
        *name = std::move(saved);
    }
    for (double coordinate : {std::numeric_limits<double>::infinity(),
                              std::numeric_limits<double>::quiet_NaN()}) {
        scene.view.center = {coordinate, 0};
        BOOST_CHECK_THROW(gui::write_scene_json(scene), std::runtime_error);
        scene.view.center = {0, coordinate};
        BOOST_CHECK_THROW(gui::write_scene_json(scene), std::runtime_error);
    }
}

BOOST_AUTO_TEST_CASE(scene_view)
{
    for (auto view : {gui::view{{540, 90}, 0.1}, {{-540, -90}, 100'000}}) {
        auto scene = gui::scene{.view = view};
        auto restored = gui::read_scene_json(gui::write_scene_json(scene));
        BOOST_TEST(restored.view.center.x() == view.center.x());
        BOOST_TEST(restored.view.center.y() == view.center.y());
        BOOST_TEST(restored.view.resolution == view.resolution);
    }
}
