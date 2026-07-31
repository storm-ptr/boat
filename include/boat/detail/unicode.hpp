// Andrew Naplavkov

#ifndef BOAT_UNICODE_HPP
#define BOAT_UNICODE_HPP

#include <boat/detail/string.hpp>
#include <boost/regex/pending/unicode_iterator.hpp>
#include <iomanip>

namespace boat::unicode {

// clang-format off
template <std::input_iterator I, std::integral T = std::iter_value_t<I>>
using unit_to_point_iter =
    std::conditional_t<sizeof(T) == 1, boost::u8_to_u32_iterator<I>,
    std::conditional_t<sizeof(T) == 2, boost::u16_to_u32_iterator<I>,
    std::conditional_t<sizeof(T) == 4, I, void>>>;

template <std::input_iterator I, std::integral T>
using point_to_unit_iter =
    std::conditional_t<sizeof(T) == 1, boost::u32_to_u8_iterator<I>,
    std::conditional_t<sizeof(T) == 2, boost::u32_to_u16_iterator<I>,
    std::conditional_t<sizeof(T) == 4, I, void>>>;
// clang-format on

constexpr auto num_points = []<std::ranges::input_range R>(R&& r) -> size_t {
    using dec = unit_to_point_iter<std::ranges::iterator_t<R>>;
    return std::distance(dec{std::ranges::begin(r)}, dec{std::ranges::end(r)});
};

template <std::integral T>
struct closure : std::ranges::range_adaptor_closure<closure<T>> {
    template <std::ranges::input_range R>
    auto operator()(R&& r) const
    {
        using dec = unit_to_point_iter<std::ranges::iterator_t<R>>;
        using enc = point_to_unit_iter<dec, T>;
        return std::ranges::subrange{enc{dec{std::ranges::begin(r)}},
                                     enc{dec{std::ranges::end(r)}}} |
               std::views::transform([](auto c) -> T { return c; });
    }
};

template <std::integral T>
constexpr auto utf = closure<T>{} | std::ranges::to<std::basic_string>();

constexpr auto utf8 = utf<char>;
constexpr auto utf16 = utf<char16_t>;
constexpr auto utf32 = utf<char32_t>;

template <std::ranges::input_range R>
struct manip {
    R range;

    template <ostream O>
    friend auto& operator<<(O& out, manip const& in)
    {
        for (auto c : closure<typename O::char_type>{}(in.range))
            out << c;
        return out;
    }
};

constexpr auto io = [](std::ranges::input_range auto&& r) {
    return manip{std::ranges::subrange{r}};
};

constexpr auto quoted =  //
    [](std::ranges::input_range auto&& r,
       char32_t delim = '"',
       char32_t escape = '\\') {
        return manip{concat<char32_t>(std::quoted(r | utf32, delim, escape))};
    };

}  // namespace boat::unicode

#endif  // BOAT_UNICODE_HPP
