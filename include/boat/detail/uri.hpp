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
        auto match = std::cmatch{};
        return std::regex_match(
                   str.data(), str.data() + str.size(), match, regex)
                   ? uri{.scheme{match[1].first, match[1].second},
                         .user{match[3].first, match[3].second},
                         .password{match[5].first, match[5].second},
                         .host_spec{match[6].first, match[6].second},
                         .path{match[8].first, match[8].second},
                         .query{match[10].first, match[10].second},
                         .fragment{match[12].first, match[12].second}}
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
