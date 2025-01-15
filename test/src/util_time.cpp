#include <chrono>

#include <catch2/catch_test_macros.hpp>

#include "util/time.hpp"

TEST_CASE("Time utilities")
{
  SECTION("known calendar days")
  {
    // Wednesday
    REQUIRE(calendar_week(std::chrono::January / 1 / 2025) == 0);
    // Monday
    REQUIRE(calendar_week(std::chrono::January / 6 / 2025) == 1);
  }
  SECTION("correct day count in years")
  {
    const auto days_in_year = [](int year)
    { return std::ranges::distance(days_of_year(std::chrono::year {year})); };

    REQUIRE(days_in_year(2025) == 365);
    // Leap year
    REQUIRE(days_in_year(2020) == 366);
    // Special rule no leap year
    REQUIRE(days_in_year(2100) == 365);
  }
  SECTION("known time point")

  {
    std::chrono::seconds const utc_duration {1736782535};
    std::chrono::utc_clock::time_point const known_date {utc_duration};
    REQUIRE(to_ymd(known_date) == std::chrono::January / 13 / 2025);
  }
}
