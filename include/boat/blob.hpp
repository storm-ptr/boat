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
    friend auto hash_value(blob const& v) { return boost::hash_value(v); }
};

template <class I, class O>
    requires std::same_as<std::remove_cvref_t<I>, blob_view>
decltype(auto) operator>>(I && in, O & out)
{
    if constexpr (arithmetic<O>) {
        check(in.size() >= sizeof out, "out of blob");
        std::memcpy(&out, in.data(), sizeof out);
        in.remove_prefix(sizeof out);
    }
    else
        static_cast<blob_view&>(in) >> out;
    return std::forward<I>(in);
}

template <class O, class I>
    requires std::same_as<std::remove_cvref_t<O>, blob>
decltype(auto) operator<<(O && out, I const& in)
{
    if constexpr (arithmetic<I>)
        out.append_range(std::as_bytes(single_span(in)));
    else
        static_cast<blob&>(out) << in;
    return std::forward<O>(out);
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
        using char_t = std::decay_t<decltype(out)>::char_type;
        auto os = std::basic_ostringstream<char_t>{};
        os << std::uppercase << std::hex << std::setfill<char_t>('0');
        for (auto byte : in.bytes)
            os << std::setw(2) << static_cast<unsigned>(byte);
        return out << os.view();  //< output once
    }
};

}  // namespace boat

template <>
struct std::hash<boat::blob> {
    static auto operator()(boat::blob const& v) { return hash_value(v); }
};

#endif  // BOAT_BLOB_HPP
