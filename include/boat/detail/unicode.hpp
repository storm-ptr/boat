// Andrew Naplavkov

#ifndef BOAT_UNICODE_HPP
#define BOAT_UNICODE_HPP

#include <boat/detail/charconv.hpp>
#include <boost/regex/pending/unicode_iterator.hpp>
#include <cwctype>
#include <iomanip>

namespace boat::unicode {

template <std::input_iterator I, std::integral Unit = std::iter_value_t<I>>
using unit_to_point_iter = std::conditional_t<
    same_size<Unit, char>,
    boost::u8_to_u32_iterator<I>,
    std::conditional_t<same_size<Unit, char16_t>,
                       boost::u16_to_u32_iterator<I>,
                       std::conditional_t<same_size<Unit, char32_t>, I, void>>>;

template <std::input_iterator I, std::integral Unit>
using point_to_unit_iter = std::conditional_t<
    same_size<Unit, char>,
    boost::u32_to_u8_iterator<I>,
    std::conditional_t<same_size<Unit, char16_t>,
                       boost::u32_to_u16_iterator<I>,
                       std::conditional_t<same_size<Unit, char32_t>, I, void>>>;

constexpr auto num_points = []<std::ranges::input_range R>(R&& r) -> size_t {
    using dec = unit_to_point_iter<std::ranges::iterator_t<R>>;
    return std::distance(dec{std::ranges::begin(r)}, dec{std::ranges::end(r)});
};

template <std::integral Unit>
struct closure : std::ranges::range_adaptor_closure<closure<Unit>> {
    template <std::ranges::input_range R>
    auto operator()(R&& r) const
    {
        using dec = unit_to_point_iter<std::ranges::iterator_t<R>>;
        using enc = point_to_unit_iter<dec, Unit>;
        return std::ranges::subrange{enc{dec{std::ranges::begin(r)}},
                                     enc{dec{std::ranges::end(r)}}} |
               std::views::transform([](auto c) -> Unit { return c; });
    }
};

template <std::integral Unit>
constexpr auto utf = closure<Unit>{} | std::ranges::to<std::basic_string>();

constexpr auto utf8 = utf<char>;
constexpr auto utf16 = utf<char16_t>;
constexpr auto utf32 = utf<char32_t>;

constexpr auto to_lower = []<std::ranges::input_range R>(R&& r) {
    return r | closure<wchar_t>{} | std::views::transform(std::towlower) |
           utf<std::ranges::range_value_t<R>>;
};

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

constexpr auto quoted = [](std::ranges::input_range auto&& r,
                           char32_t delim = '"',
                           char32_t escape = '\\') {
    return manip{concat<char32_t>(std::quoted(r | utf32, delim, escape))};
};

}  // namespace boat::unicode

#endif  // BOAT_UNICODE_HPP
