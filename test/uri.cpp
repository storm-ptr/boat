// Andrew Naplavkov

#include <boat/detail/uri.hpp>
#include <boat/detail/utility.hpp>
#include <boost/test/unit_test.hpp>

BOOST_AUTO_TEST_CASE(uri)
{
    struct {
        std::string_view str;
        boat::uri expect;
    } tests[] = {
        {"odbc://sa:E207cGYM@192.168.31.128/master?DRIVER=SQL Server",
         {.scheme = "odbc",
          .user = "sa",
          .password = "E207cGYM",
          .host = "192.168.31.128",
          .path = "master",
          .query = "DRIVER=SQL Server"}},
        {"postgresql://postgres:E207cGYM@192.168.31.128:5432/"
         "postgres?client_encoding=UTF8",
         {.scheme = "postgresql",
          .user = "postgres",
          .password = "E207cGYM",
          .host = "192.168.31.128",
          .port = 5432,
          .path = "postgres",
          .query = "client_encoding=UTF8"}},
        {"sqlite:///:memory:", {.scheme = "sqlite", .path = ":memory:"}},
        {"slippy://algol@", {.scheme = "slippy", .user = "algol"}},
    };
    for (auto& [str, expect] : tests) {
        auto uri = boat::parse_uri(str);
        BOOST_CHECK(uri == expect);
        BOOST_CHECK(concat(*uri) == str);
    }
}
