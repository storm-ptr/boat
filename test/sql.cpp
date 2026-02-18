// Andrew Naplavkov

#include <boat/sql/api.hpp>
#include <boat/sql/io.hpp>
#include <boost/test/unit_test.hpp>
#include <iostream>
#include "commands.hpp"
#include "data.hpp"

using namespace boat;

BOOST_AUTO_TEST_CASE(sql_api)
{
    auto objs = get_objects();
    auto page = sql::page{
        .select_list = boost::pfr::names_as_array<object_struct>() |
                       std::ranges::to<std::vector<std::string>>(),
        .limit = static_cast<int>(std::ranges::size(objs)),
    };
    auto bbox = sql::bbox{
        .select_list = {std::string(boost::pfr::get_name<0, object_struct>())},
        .xmin = 9,
        .ymin = 9,
        .xmax = 11,
        .ymax = 11,
        .limit = int(std::ranges::size(objs))};
    auto draft = get_object_table();
    for (auto cmd : commands()) {
        cmd->set_autocommit(false);
        cmd->exec({"drop table if exists ", db::id{draft.table_name}});
        auto tbl = sql::create(*cmd, draft);
        std::cout << tbl;
        auto rows = boat::pfr::to_rowset(objs);
        sql::insert(*cmd, tbl, rows);
        BOOST_CHECK(std::ranges::equal(
            objs,
            sql::select(*cmd, tbl, page) | pfr::view<object_struct>,
            BOAT_LIFT(boost::pfr::eq_fields)));
        BOOST_CHECK(std::ranges::equal(
            std::array{2}, sql::select(*cmd, tbl, bbox) | pfr::view<int>));
    }
}

namespace {

constexpr auto mssql_datatypes = R"(
create table "datatypes" (
    "bigint" bigint,
    "binary" binary(8),
    "bit" bit,
    "char" char(8),
    "datetime2" datetime2(7),
    "datetimeoffset" datetimeoffset(7),
    "decimal" decimal(5, 2),
    "float" float(53),
    "int" int,
    "nchar" nchar(8),
    "numeric" numeric(10, 5),
    "nvarchar" nvarchar(max),
    "real" real,
    "smallint" smallint,
    "tinyint" tinyint,
    "varbinary" varbinary(max),
    "varchar" varchar(max));
insert into "datatypes" values (
    9223372036854775807,
    cast(123456 as binary(4)),
    1,
    'abc',
    '2007-05-08 12:35:29.1234567',
    '2007-05-08 12:35:29.1234567 +12:15',
    123.45,
    1234567.89012345,
    2147483647,
    'def',
    12345.67891,
    'ghi',
    123.4567,
    32767,
    255,
    cast(123456 as varbinary(4)),
    'jkl');)";

constexpr auto mysql_datatypes = R"(
create table `datatypes` (
    `bigint` bigint,
    `binary` binary(3),
    `blob` blob,
    `char` char(4),
    `datetime` datetime(6),
    `decimal` decimal(5, 2),
    `double` double,
    `float` float(23),
    `int` int,
    `longblob` longblob,
    `longtext` longtext,
    `mediumblob` mediumblob,
    `mediumint` mediumint,
    `mediumtext` mediumtext,
    `smallint` smallint,
    `text` text,
    `timestamp` timestamp,
    `tinyblob` tinyblob,
    `tinyint` tinyint,
    `tinytext` tinytext,
    `varbinary` varbinary(3),
    `varchar` varchar(4));
insert into `datatypes` values (
    9223372036854775807,
    'b',
    'blob',
    'abcd',
    '2020-01-01 10:10:10.499999',
    123.45,
    1234567.89012345,
    1234.5,
    2147483647,
    'longblob',
    'longtext',
    'mediumblob',
    8388607,
    'mediumtext',
    32767,
    'text',
    '2019-12-31 23:40:10',
    'tinyblob',
    127,
    'tinytext',
    'b',
    'abcd');)";

constexpr auto postgresql_datatypes = R"(
create table "datatypes" (
    "bigint" bigint,
    -- bigserial,
    -- boolean,
    "bytea" bytea,
    "character" character(32),
    "character varying" character varying(32),
    "double precision" double precision,
    "integer" integer,
    "numeric" numeric(5, 2),
    "real" real,
    -- serial,
    "smallint" smallint,
    -- smallserial,
    "text" text,
    -- timestamp (6) with time zone,
    "timestamp without time zone" timestamp (6) without time zone);
insert into "datatypes" values (
    9223372036854775807,
    '\xDEADBEEF'::bytea,
    'character',
    'character varying',
    1234567.89012345,
    2147483647,
    123.45,
    1234.5,
    32767,
    'text',
    '2004-10-19 10:23:54.123456');)";

constexpr auto sqlite_datatypes = R"(
create table "datatypes" (
    "blob" blob,
    "integer" integer,
    "numeric" numeric,
    "real" real,
    "text" text,
    "timestamp" timestamp);
insert into "datatypes" values (
    x'DEADBEEF',
    2147483647,
    123.45,
    1234567.89012345,
    'text',
    '2004-10-19 10:23:54.123');)";

auto datatypes_query(std::string_view lcase_dbms)
{
    return  //
        lcase_dbms.contains(sql::mssql_dbms)        ? mssql_datatypes
        : lcase_dbms.contains(sql::mysql_dbms)      ? mysql_datatypes
        : lcase_dbms.contains(sql::postgresql_dbms) ? postgresql_datatypes
        : lcase_dbms.contains(sql::sqlite_dbms)     ? sqlite_datatypes
                                                    : "";
}

}  // namespace

BOOST_AUTO_TEST_CASE(sql_datatypes)
{
    auto tbl_a_name = "datatypes";
    auto tbl_b_name = "datatypes_copy";
    auto page = sql::page{.limit = 1};
    for (auto cmd : commands()) {
        cmd->set_autocommit(false);
        cmd->exec({"drop table if exists ", db::id{tbl_a_name}});
        cmd->exec({"drop table if exists ", db::id{tbl_b_name}});
        cmd->exec(datatypes_query(cmd->lcase_dbms()));
        auto tbl_a = sql::describe(*cmd, tbl_a_name);
        auto rows = sql::select(*cmd, tbl_a, page);
        BOOST_CHECK_EQUAL(tbl_a.columns.size(), rows.columns.size());
        BOOST_CHECK(!rows.empty());
        auto tbl_b = tbl_a;
        tbl_b.table_name = tbl_b_name;
        tbl_b = sql::create(*cmd, tbl_b);
        sql::insert(*cmd, tbl_b, rows);

        rows = cmd->exec({
            "select ",
            sql::select_list{tbl_a},
            " from ",
            db::id{tbl_a_name},
            " union all select ",
            sql::select_list{tbl_b},
            " from ",
            db::id{tbl_b_name},
        });
        BOOST_CHECK_EQUAL(rows.rows.size(), 2u);
        for (auto&& [col, val_a, val_b] :
             std::views::zip(rows.columns, rows.rows.at(0), rows.rows.at(1)))
            BOOST_TEST((val_a == val_b), col);

        rows = cmd->exec({
            "select count(*) from (select * from ",
            db::id{tbl_a_name},
            " except select * from ",
            db::id{tbl_b_name},
            ") as t",
        });
        BOOST_CHECK_EQUAL(pfr::get<int>(rows.value()), 0);
    }
}
