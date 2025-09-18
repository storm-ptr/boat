// Andrew Naplavkov

#ifndef BOAT_PFR_ADAPTED_TIME_POINT_HPP
#define BOAT_PFR_ADAPTED_TIME_POINT_HPP

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
    auto match = std::cmatch{};
    check(std::regex_match(str.data(), str.data() + str.size(), match, regex),
          str);
    auto ymd = sc::year_month_day(
        sc::year(from_chars<int>(match[1].first, match[1].length())),
        sc::month(from_chars<int>(match[2].first, match[2].length())),
        sc::day(from_chars<int>(match[3].first, match[3].length())));
    auto h = from_chars<int>(match[4].first, match[4].length());
    auto min = from_chars<int>(match[5].first, match[5].length());
    auto s = from_chars<int>(match[6].first, match[6].length());
    auto us = static_cast<int>(
        (match[8].length()
             ? from_chars<int>(match[8].first, match[8].length()) /
                   std::pow(10, match[8].length())
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
    auto d = sc::floor<sc::days>(epoch);
    auto ymd = sc::year_month_day(sc::sys_days(d));
    auto y = static_cast<int>(ymd.year());
    check(y >= 0 && y <= 9999, concat(y, " year"));
    auto h = sc::floor<sc::hours>(epoch);
    auto min = sc::floor<sc::minutes>(epoch);
    auto us = sc::floor<sc::microseconds>(epoch);
    auto s = static_cast<double>((us - min).count()) /
             sc::microseconds::period::den * sc::microseconds::period::num;
    auto os = std::ostringstream{};
    os.imbue(std::locale::classic());
    os << std::setfill('0') << std::setw(4) << y << "-" << std::setw(2)
       << static_cast<unsigned>(ymd.month()) << "-" << std::setw(2)
       << static_cast<unsigned>(ymd.day()) << " " << std::setw(2)
       << (h - d).count() << ":" << std::setw(2) << (min - h).count() << ":"
       << (s < 10 ? "0" : "") << s;
    out = std::move(os).str();
}

}  // namespace boat::pfr

#endif  // BOAT_PFR_ADAPTED_TIME_POINT_HPP
