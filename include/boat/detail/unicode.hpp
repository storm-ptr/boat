// Andrew Naplavkov

#ifndef BOAT_UNICODE_HPP
#define BOAT_UNICODE_HPP

#include <boat/detail/charconv.hpp>
#include <boost/regex/pending/unicode_iterator.hpp>
#include <cwctype>
#include <iomanip>

namespace boat::unicode {

using point = char32_t;

template <std::input_iterator I, class Unit = std::iter_value_t<I>>
using unit_to_point_iter = std::conditional_t<
    same_size<Unit, char8_t>,
    boost::u8_to_u32_iterator<I>,
    std::conditional_t<same_size<Unit, char16_t>,
                       boost::u16_to_u32_iterator<I>,
                       std::conditional_t<same_size<Unit, point>, I, void>>>;

template <std::input_iterator I, class Unit>
using point_to_unit_iter = std::conditional_t<
    same_size<Unit, char8_t>,
    boost::u32_to_u8_iterator<I>,
    std::conditional_t<same_size<Unit, char16_t>,
                       boost::u32_to_u16_iterator<I>,
                       std::conditional_t<same_size<Unit, point>, I, void>>>;

size_t num_points(std::ranges::input_range auto&& r)
{
    using decode = unit_to_point_iter<std::ranges::iterator_t<decltype(r)>>;
    return std::distance(decode{std::ranges::begin(r)},
                         decode{std::ranges::end(r)});
}

template <class Unit>
struct closure : std::ranges::range_adaptor_closure<closure<Unit>> {
    auto operator()(std::ranges::input_range auto&& r) const
    {
        using decode = unit_to_point_iter<std::ranges::iterator_t<decltype(r)>>;
        using encode = point_to_unit_iter<decode, Unit>;
        return std::ranges::subrange{encode{decode{std::ranges::begin(r)}},
                                     encode{decode{std::ranges::end(r)}}} |
               std::views::transform([](auto c) -> Unit { return c; });
    }
};

template <class Unit>
constexpr auto view = closure<Unit>{};

template <class Unit>
constexpr auto string = view<Unit> | std::ranges::to<std::basic_string>();

static auto const lower = view<wchar_t> | std::views::transform(std::towlower);

static auto const upper = view<wchar_t> | std::views::transform(std::towupper);

template <std::ranges::input_range T>
struct manip {
    T range;

    friend auto& operator<<(ostream auto& out, manip const& in)
    {
        using char_type = std::decay_t<decltype(out)>::char_type;
        for (auto c : view<char_type>(in.range))
            out << c;
        return out;
    }
};

constexpr auto io = [](std::ranges::input_range auto&& r) {
    return manip{std::ranges::subrange{r}};
};

constexpr auto quoted = [](std::ranges::input_range auto&& r,
                           point delim = '"',
                           point escape = '\\') {
    return manip{concat<point>(std::quoted(r | string<point>, delim, escape))};
};

}  // namespace boat::unicode

#endif  // BOAT_UNICODE_HPP
