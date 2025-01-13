#pragma once
#include <chrono>
#include <ranges>

inline auto to_ymd(const std::chrono::utc_clock::time_point& input_time)
    -> std::chrono::year_month_day
{
  return std::chrono::year_month_day {std::chrono::floor<std::chrono::days>(
      std::chrono::system_clock::time_point((input_time.time_since_epoch())))};
}

/**
  Get the zero-indexed calendar week of the year which contains the given day
 */
inline auto calendar_week(const std::chrono::year_month_day& day) -> unsigned int
{
  constexpr int days_in_week = 7;
  // Number of days in the first week of the year
  const std::chrono::days second_week_delta {
      days_in_week
      - (std::chrono::weekday {std::chrono::January / 01 / day.year()}
             .iso_encoding()
         - 1)};
  const std::chrono::year_month_day second_week_start {
      std::chrono::January / static_cast<int>(1 + second_week_delta.count())
      / day.year()};
  if (second_week_start > day) {
    return 0;
  }
  const std::chrono::days day_delta =
      std::chrono::sys_days(day) - std::chrono::sys_days(second_week_start);
  return static_cast<unsigned int>(day_delta.count() / days_in_week) + 1;
}

/**
Return iterator of all days contained in the year
*/
inline auto days_of_year(std::chrono::year year)
{
  return std::views::iota(0)
      | std::views::transform(
             [year](auto day_delta)
             {
               return std::chrono::sys_days {std::chrono::January / 01 / year}
               + std::chrono::days {day_delta};
             })
      | std::views::take_while(
             [year](auto day)
             { return std::chrono::year_month_day {day}.year() == year; });
}
