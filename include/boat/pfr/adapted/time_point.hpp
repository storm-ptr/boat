// Andrew Naplavkov

#ifndef BOAT_PFR_ADAPTED_TIME_POINT_HPP
#define BOAT_PFR_ADAPTED_TIME_POINT_HPP

#include <boat/pfr/variant.hpp>
#include <chrono>
#include <regex>

namespace boat::pfr {

// commented to avoid std::system_error: The specified module could not be found

namespace detail {

template <class T>
T from_chars(auto const& val)
{
    return boat::from_chars<T>(val.first, val.length());
}

}  // namespace detail

template <class... Ts>
void read(variant const& in, std::chrono::time_point<Ts...>& out)
{
    using time_point = std::decay_t<decltype(out)>;
    using duration = time_point::duration;
    static auto const regex =
        std::regex{R"(^(\d{4})-(\d{2})-(\d{2}) (\d{2}):(\d{2}):(\d{2})(\.\d+)?$)"};
    if (in.has_value()) {
        auto& str = std::get<std::string>(in);
        auto match = std::cmatch{};
        check(
            std::regex_match(str.data(), str.data() + str.size(), match, regex),
            str.data());
        auto ymd = std::chrono::year_month_day{
            std::chrono::year{detail::from_chars<int>(match[1])},
            std::chrono::month{detail::from_chars<unsigned>(match[2])},
            std::chrono::day{detail::from_chars<unsigned>(match[3])}};
        auto h = detail::from_chars<unsigned>(match[4]);
        auto m = detail::from_chars<unsigned>(match[5]);
        auto s = detail::from_chars<unsigned>(match[6]);
        check(h < 24 && m < 60 && s < 60, str.data());
        out = time_point(std::chrono::duration_cast<duration>(
            std::chrono::sys_days(ymd).time_since_epoch()));
        out += std::chrono::hours{h} + std::chrono::minutes{m} +
               std::chrono::seconds{s};

        // std::istringstream{std::get<std::string>(in)} >>
        //     std::chrono::parse("%F %T", out);
    }
    else
        out = {};
}

template <class... Ts>
void write(variant& out, std::chrono::time_point<Ts...> in)
{
    auto epoch = in.time_since_epoch();
    auto d = std::chrono::floor<std::chrono::days>(epoch);
    auto h = std::chrono::floor<std::chrono::hours>(epoch);
    auto m = std::chrono::floor<std::chrono::minutes>(epoch);
    auto s = std::chrono::floor<std::chrono::seconds>(epoch);
    auto ymd = std::chrono::year_month_day(std::chrono::sys_days(d));
    auto os = std::ostringstream{};
    os.imbue(std::locale::classic());
    os << std::setfill('0') << std::setw(4) << static_cast<int>(ymd.year())
       << "-" << std::setw(2) << static_cast<unsigned>(ymd.month()) << "-"
       << std::setw(2) << static_cast<unsigned>(ymd.day()) << " "
       << std::setw(2) << (h - d).count() << ":" << std::setw(2)
       << (m - h).count() << ":" << std::setw(2) << (s - m).count();
    out = std::move(os).str();

    // out = concat(std::format("{:L%F %T}", in));
}

}  // namespace boat::pfr

#endif  // BOAT_PFR_ADAPTED_TIME_POINT_HPP
