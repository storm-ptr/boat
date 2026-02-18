// Andrew Naplavkov

#ifndef BOAT_GDAL_ADAPTED_DATE_TIME_HPP
#define BOAT_GDAL_ADAPTED_DATE_TIME_HPP

#include <boat/gdal/detail/utility.hpp>
#include <chrono>

namespace boat::gdal {

using duration = std::chrono::duration<double>;
using time_point = std::chrono::time_point<std::chrono::system_clock, duration>;

inline time_point get_date_time(OGRFeatureH feat, int index)
{
    namespace sc = std::chrono;
    int year, month, day, hour, minute, tz;
    float second;
    boat::check(                     //
        OGR_F_GetFieldAsDateTimeEx(  //
            feat,
            index,
            &year,
            &month,
            &day,
            &hour,
            &minute,
            &second,
            &tz),
        "OGR_F_GetFieldAsDateTimeEx");
    auto ymd =
        sc::year_month_day(sc::year(year), sc::month(month), sc::day(day));
    return time_point(sc::sys_days(ymd).time_since_epoch() + sc::hours(hour) +
                      sc::minutes(minute) + duration(second));
}

inline void set_date_time(OGRFeatureH feat, int index, time_point tp)
{
    namespace sc = std::chrono;
    auto epoch = tp.time_since_epoch();
    auto days = sc::floor<sc::days>(epoch);
    auto ymd = sc::year_month_day{sc::sys_days(days)};
    auto hms = sc::hh_mm_ss{sc::duration_cast<duration>(epoch - days)};
    OGR_F_SetFieldDateTimeEx(  //
        feat,
        index,
        static_cast<int>(ymd.year()),
        static_cast<unsigned>(ymd.month()),
        static_cast<unsigned>(ymd.day()),
        hms.hours().count(),
        hms.minutes().count(),
        static_cast<float>(hms.seconds().count() + hms.subseconds().count()),
        100);
}

}  // namespace boat::gdal

#endif  // BOAT_GDAL_ADAPTED_DATE_TIME_HPP
