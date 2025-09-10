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
        {"mysql://root:Password12!@localhost:3306/mysql",
         {.scheme = "mysql",
          .user = "root",
          .password = "Password12!",
          .host_spec = "localhost:3306",
          .path = "mysql"}},
        {"mysql://user@localhost:3306?get-server-public-key=true",
         {.scheme = "mysql",
          .user = "user",
          .host_spec = "localhost:3306",
          .query = "get-server-public-key=true"}},
        {"mysqlx://user_name@server.example.com/",
         {.scheme = "mysqlx",
          .user = "user_name",
          .host_spec = "server.example.com"}},
        {"mysqlx://user_name@198.51.100.14:123",
         {.scheme = "mysqlx",
          .user = "user_name",
          .host_spec = "198.51.100.14:123"}},
        {"mysqlx://user_name@[2001:db8:85a3:8d3:1319:8a2e:370:7348]",
         {.scheme = "mysqlx",
          .user = "user_name",
          .host_spec = "[2001:db8:85a3:8d3:1319:8a2e:370:7348]"}},
        {"odbc://sa:Password12!@(local)\\SQL2019/master?DRIVER=SQL Server",
         {.scheme = "odbc",
          .user = "sa",
          .password = "Password12!",
          .host_spec = "(local)\\SQL2019",
          .path = "master",
          .query = "DRIVER=SQL Server"}},
        {"postgresql://postgres:Password12!@localhost:5432/"
         "postgres?client_encoding=UTF8",
         {.scheme = "postgresql",
          .user = "postgres",
          .password = "Password12!",
          .host_spec = "localhost:5432",
          .path = "postgres",
          .query = "client_encoding=UTF8"}},
        {"postgresql://localhost",
         {.scheme = "postgresql", .host_spec = "localhost"}},
        {"postgresql://localhost:5433",
         {.scheme = "postgresql", .host_spec = "localhost:5433"}},
        {"postgresql://localhost/mydb",
         {.scheme = "postgresql", .host_spec = "localhost", .path = "mydb"}},
        {"postgresql://user@localhost",
         {.scheme = "postgresql", .user = "user", .host_spec = "localhost"}},
        {"postgresql://user:secret@localhost",
         {.scheme = "postgresql",
          .user = "user",
          .password = "secret",
          .host_spec = "localhost"}},
        {"postgresql://other@localhost/"
         "otherdb?connect_timeout=10&application_name=myapp",
         {.scheme = "postgresql",
          .user = "other",
          .host_spec = "localhost",
          .path = "otherdb",
          .query = "connect_timeout=10&application_name=myapp"}},
        {"postgresql://host1:123,host2:456/"
         "somedb?target_session_attrs=any&application_name=myapp",
         {.scheme = "postgresql",
          .host_spec = "host1:123,host2:456",
          .path = "somedb",
          .query = "target_session_attrs=any&application_name=myapp"}},
        {"sqlite:///:memory:", {.scheme = "sqlite", .path = ":memory:"}},
        {"slippy://algol@", {.scheme = "slippy", .user = "algol"}},
    };
    for (auto& [str, expect] : tests)
        BOOST_CHECK(boat::uri::parse(str) == expect);
}
