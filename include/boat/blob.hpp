// Andrew Naplavkov

#ifndef BOAT_BLOB_HPP
#define BOAT_BLOB_HPP

#include <boat/detail/utility.hpp>
#include <boost/functional/hash.hpp>
#include <cstring>
#include <iomanip>
#include <ios>

namespace boat {

struct blob_view : std::basic_string_view<std::byte> {
    using std::basic_string_view<std::byte>::basic_string_view;
    blob_view(std::byte const*) = delete;
};

struct blob : std::basic_string<std::byte> {
    using std::basic_string<std::byte>::basic_string;
    blob(std::byte const*) = delete;
    operator blob_view() const { return {data(), size()}; }
};

blob_view& operator>>(blob_view& in, arithmetic auto& out)
{
    check(in.size() >= sizeof out, "out of blob");
    std::memcpy(&out, in.data(), sizeof out);
    in.remove_prefix(sizeof out);
    return in;
}

blob_view&& operator>>(blob_view&& in, auto& out)
    requires requires { in >> out; }
{
    in >> out;
    return std::move(in);
}

blob& operator<<(blob& out, arithmetic auto const& in)
{
    out.append_range(std::as_bytes(single_span(in)));
    return out;
}

blob&& operator<<(blob&& out, auto const& in)
    requires requires { out << in; }
{
    out << in;
    return std::move(out);
}

template <class T>
T get(blob_view& in)
    requires requires(T out) { in >> out; }
{
    T ret;
    in >> ret;
    return ret;
}

template <arithmetic T>
T get(blob_view& in, std::endian e)
    requires(!mixed(std::endian::native))
{
    check(!mixed(e), "mixed endian");
    T ret;
    in >> ret;
    if (std::endian::native != e)
        ret = byteswap(ret);
    return ret;
}

struct hex {
    blob_view bytes;

    friend auto& operator<<(ostream auto& out, hex const& in)
    {
        using char_type = std::decay_t<decltype(out)>::char_type;
        auto os = std::basic_ostringstream<char_type>{};
        os << std::uppercase << std::hex << std::setfill<char_type>('0');
        for (auto byte : in.bytes)
            os << std::setw(2) << static_cast<unsigned>(byte);
        return out << os.view();  //< output once
    }
};

}  // namespace boat

template <>
struct std::hash<boat::blob> {
    static size_t operator()(boat::blob const& that)
    {
        return boost::hash_range(that.begin(), that.end());
    }
};

#endif  // BOAT_BLOB_HPP
