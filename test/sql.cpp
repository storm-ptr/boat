// Andrew Naplavkov

#include <boat/sql/dal.hpp>
#include <boost/test/unit_test.hpp>
#include "commands.hpp"
#include "data.hpp"

using namespace boat;

BOOST_AUTO_TEST_CASE(sql_select)
{
    struct udt {
        int64_t n;
        std::optional<double> d;
        std::string s;
    };
    auto expect = std::vector<udt>{{.s{"a"}}, {.n = 1, .d = 3.14, .s{"b"}}};
    auto qry = db::query{"select 0, null, 'a' union select 1, 3.14, 'b'"};
    for (auto cmd : commands())
        BOOST_CHECK(std::ranges::equal(  //
            expect,
            cmd->exec(qry) | db::view<udt>,
            BOAT_LIFT(boost::pfr::eq_fields)));
}

BOOST_AUTO_TEST_CASE(sql_param)
{
    auto objs = get_objects();
    auto qry = db::query{};
    for (auto sep1{"\n select "}; auto& row : db::to_rowset(objs)) {
        qry << std::exchange(sep1, "\n union select ");
        for (auto sep2{""}; auto& var : row)
            qry << std::exchange(sep2, ", ") << var;
    }
    for (auto cmd : commands())
        BOOST_CHECK(std::ranges::equal(  //
            objs,
            cmd->exec(qry) | db::view<udt>,
            BOAT_LIFT(boost::pfr::eq_fields)));
}

BOOST_AUTO_TEST_CASE(sql_agent)
{
    for (auto cmd : commands()) {
        auto dal = sql::dal{};
        dal.command = std::move(cmd);
        dal.command->set_autocommit(false);
        check(dal);
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

constexpr auto postgres_datatypes = R"(
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

auto datatypes_query(std::string_view dbms)
{
    return sql::is_mssql(dbms)      ? mssql_datatypes
           : sql::is_mysql(dbms)    ? mysql_datatypes
           : sql::is_postgres(dbms) ? postgres_datatypes
           : sql::is_sqlite(dbms)   ? sqlite_datatypes
                                    : "";
}

}  // namespace

BOOST_AUTO_TEST_CASE(sql_datatypes)
{
    auto tbl_a_name = "datatypes";
    auto tbl_b_name = "datatypes_copy";
    auto page = db::page{.limit = 1};
    for (auto cmd : commands()) {
        auto dal = sql::dal{};
        dal.command = std::move(cmd);
        dal.command->set_autocommit(false);
        dal.drop("", tbl_a_name);
        dal.drop("", tbl_b_name);
        dal.command->exec(datatypes_query(dal.command->dbms()));
        auto tbl_a = dal.get_table("", tbl_a_name);
        auto rows = dal.select(tbl_a, page);
        BOOST_CHECK_EQUAL(tbl_a.columns.size(), rows.columns.size());
        BOOST_CHECK(!rows.empty());
        auto tbl_b = tbl_a;
        tbl_b.table_name = tbl_b_name;
        tbl_b = dal.create(tbl_b);
        dal.insert(tbl_b, rows);
        rows = dal.command->exec({
            "select count(*) from (select * from ",
            sql::id{tbl_a},
            " except select * from ",
            sql::id{tbl_b},
            ") as t",
        });
        BOOST_CHECK_EQUAL(db::get<int>(rows.value()), 0);
    }
}
