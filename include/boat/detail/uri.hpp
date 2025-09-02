// Andrew Naplavkov

#ifndef BOAT_URI_HPP
#define BOAT_URI_HPP

#include <cstdlib>
#include <locale>
#include <optional>
#include <ranges>
#include <regex>
#include <sstream>

namespace boat {

struct uri {
    std::string scheme;
    std::string user;
    std::string password;
    std::string host;
    int port;
    std::string path;
    std::string query;
    std::string fragment;

    friend auto operator<=>(uri const&, uri const&) = default;

    friend auto& operator<<(std::ostream& out, uri const& in)
    {
        out << in.scheme << "://";
        if (!in.user.empty()) {
            out << in.user;
            if (!in.password.empty())
                out << ':' << in.password;
            out << '@';
        }
        out << in.host;
        if (in.port)
            out << ':' << in.port;
        if (!in.path.empty())
            out << '/' << in.path;
        if (!in.query.empty())
            out << '?' << in.query;
        if (!in.fragment.empty())
            out << '#' << in.fragment;
        return out;
    }
};

inline std::optional<uri> parse_uri(std::string_view str)
{
    static auto const regex = std::regex{
        R"(^(\w+):\/\/(([^:@]*)(:([^@]*))?\@)?(([\w\d\.\(\)\\-]+)(:(\d+))?)?(\/([\w\d\.\/\-\:]*))?(\?([\w\d\s\.\=\&]*))?(\#[\w\d\-]*)?$)"};
    auto match = std::cmatch{};
    return std::regex_match(str.data(), str.data() + str.size(), match, regex)
               ? std::optional{uri{.scheme = match[1],
                                   .user = match[3],
                                   .password = match[5],
                                   .host = match[7],
                                   .port = std::atoi(match[9].str().data()),
                                   .path = match[11],
                                   .query = match[13],
                                   .fragment = match[14]}}
               : std::nullopt;
}

inline std::string to_string(uri const& val)
{
    auto os = std::ostringstream{};
    os.imbue(std::locale::classic());
    if ("odbc" == val.scheme) {
        if (!val.user.empty())
            os << "uid=" << val.user << ';';
        if (!val.password.empty())
            os << "pwd=" << val.password << ';';
        if (!val.host.empty())
            os << "server=" << val.host << ';';
        if (val.port)
            os << "port=" << val.port << ';';
        if (!val.path.empty())
            os << "database=" << val.path << ';';
        for (char c : val.query | std::views::transform([](char c) {
                          return c == '&' ? ';' : c;
                      }))
            os << c;
    }
    else if ("file" == val.scheme || "sqlite" == val.scheme)
        os << val.path;
    else
        os << val;
    return std::move(os).str();
}

}  // namespace boat

#endif  // BOAT_URI_HPP
