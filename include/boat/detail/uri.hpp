// Andrew Naplavkov

#ifndef BOAT_URI_HPP
#define BOAT_URI_HPP

#include <regex>
#include <stdexcept>

namespace boat {

struct uri {
    std::string_view scheme;
    std::string_view user;
    std::string_view password;
    std::string_view host_spec;  //< [host][:port][,...]
    std::string_view path;
    std::string_view query;
    std::string_view fragment;

    friend bool operator==(uri const&, uri const&) = default;

    static uri parse(std::string_view str)
    {
        static auto const regex = std::regex{
            R"(^(\w+):\/\/(([^:@]*)(:([^@]*))?\@)?([^/?#]+)?(\/([^?#]*))?(\?([^#]*))?(\#(.*))?$)"};
        auto m = std::cmatch{};
        return std::regex_match(str.data(), str.data() + str.size(), m, regex)
                   ? uri{.scheme{m[1].first, m[1].second},
                         .user{m[3].first, m[3].second},
                         .password{m[5].first, m[5].second},
                         .host_spec{m[6].first, m[6].second},
                         .path{m[8].first, m[8].second},
                         .query{m[10].first, m[10].second},
                         .fragment{m[12].first, m[12].second}}
                   : throw std::runtime_error("uri::parse");
    }
};

struct socket_address {
    std::string_view host;
    std::string_view port;

    static socket_address parse(std::string_view host_spec)
    {
        if (host_spec.contains(','))
            return {host_spec};
        auto pos = host_spec.find_last_not_of("0123456789");
        if (pos == std::string::npos || host_spec[pos] != ':')
            return {host_spec};
        auto host = host_spec.substr(0, pos);
        auto port = host_spec.substr(pos + 1);
        if (host.empty() || port.empty())
            return {host_spec};
        return {host, port};
    }
};

}  // namespace boat

#endif  // BOAT_URI_HPP
