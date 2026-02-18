// Andrew Naplavkov

#ifndef BOAT_PFR_ADAPTED_TIME_POINT_HPP
#define BOAT_PFR_ADAPTED_TIME_POINT_HPP

#include <boat/detail/algorithm.hpp>
#include <boat/detail/charconv.hpp>
#include <boat/pfr/variant.hpp>
#include <chrono>
#include <regex>

namespace boat::pfr {

template <class Clock, class Duration>
void read(variant const& in, std::chrono::time_point<Clock, Duration>& out)
{
    if (!in.has_value()) {
        out = {};
        return;
    }
    // std::chrono::parse("%F %T", out)
    namespace sc = std::chrono;
    static auto const regex = std::regex{
        R"(^(\d{4})-(\d{2})-(\d{2}) (\d{2}):(\d{2}):(\d{2})(\.(\d+))?$)"};
    auto& str = std::get<std::string>(in);
    auto m = std::cmatch{};
    check(std::regex_match(str.data(), str.data() + str.size(), m, regex), str);
    auto ymd = sc::year_month_day(
        sc::year(from_chars<int>(m[1].first, m[1].length())),
        sc::month(from_chars<int>(m[2].first, m[2].length())),
        sc::day(from_chars<int>(m[3].first, m[3].length())));
    auto h = from_chars<int>(m[4].first, m[4].length());
    auto min = from_chars<int>(m[5].first, m[5].length());
    auto s = from_chars<int>(m[6].first, m[6].length());
    auto us = static_cast<int>(
        (m[8].length() ? from_chars<int>(m[8].first, m[8].length()) * 1. /
                             ipow(10u, m[8].length())
                       : 0) *
        sc::microseconds::period::den / sc::microseconds::period::num);
    check(ymd.ok() && h < 24 && min < 60 && s < 60, str.data());
    out = sc::time_point_cast<Duration>(
        sc::time_point<Clock, sc::microseconds>{
            sc::duration_cast<sc::microseconds>(
                sc::sys_days(ymd).time_since_epoch())} +
        sc::hours{h} + sc::minutes{min} + sc::seconds{s} +
        sc::microseconds{us});
}

template <class Clock, class Duration>
void write(variant& out, std::chrono::time_point<Clock, Duration> in)
{
    // std::format("{:L%F %T}", in)
    namespace sc = std::chrono;
    auto epoch = in.time_since_epoch();
    auto days = sc::floor<sc::days>(epoch);
    auto ymd = sc::year_month_day{sc::sys_days(days)};
    auto hms =
        sc::hh_mm_ss{sc::duration_cast<sc::duration<double>>(epoch - days)};
    auto os = std::ostringstream{};
    os.imbue(std::locale::classic());
    os << std::setfill('0') << std::setw(4) << static_cast<int>(ymd.year())
       << "-" << std::setw(2) << static_cast<unsigned>(ymd.month()) << "-"
       << std::setw(2) << static_cast<unsigned>(ymd.day()) << " "
       << std::setw(2) << hms.hours().count() << ":" << std::setw(2)
       << hms.minutes().count() << ":"
       << (hms.seconds().count() < 10 ? "0" : "")
       << hms.seconds().count() + hms.subseconds().count();
    out = std::move(os).str();
}

}  // namespace boat::pfr

#endif  // BOAT_PFR_ADAPTED_TIME_POINT_HPP
