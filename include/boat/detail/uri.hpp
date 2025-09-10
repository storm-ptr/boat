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
        static auto const adapt = [](auto const& val) {
            return std::string_view{val.first, val.second};
        };
        auto match = std::cmatch{};
        return std::regex_match(
                   str.data(), str.data() + str.size(), match, regex)
                   ? uri{.scheme = adapt(match[1]),
                         .user = adapt(match[3]),
                         .password = adapt(match[5]),
                         .host_spec = adapt(match[6]),
                         .path = adapt(match[8]),
                         .query = adapt(match[10]),
                         .fragment = adapt(match[12])}
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
