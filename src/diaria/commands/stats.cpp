#include <cassert>
#include <chrono>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <print>
#include <ranges>
#include <span>

#include <bits/chrono.h>
#include <sys/types.h>

#include "diaria/commands.hpp"
#include "diaria/repo_management.hpp"

auto to_ymd(const auto& tp)
{
  return std::chrono::year_month_day {std::chrono::floor<std::chrono::days>(
      std::chrono::system_clock::time_point((tp.time_since_epoch())))};
}

auto handle_year(const std::ranges::forward_range auto& entries,
                 std::chrono::year year)
    -> std::pair<std::size_t, std::vector<std::uint32_t>>
{
  auto relevant_days =
      entries
      | std::views::drop_while([&](const auto& p)
                               { return to_ymd(p.first).year() < year; })
      | std::ranges::views::take_while(
          [&](const auto& p) { return to_ymd(p.first).year() == year; });
  auto current_day = relevant_days.begin();
  std::size_t entry_count {};
  std::vector<std::uint32_t> cells {};
  for (std::chrono::sys_days i {std::chrono::January / 01 / year};
       std::chrono::year_month_day {i}.year() == year
       && i < std::chrono::system_clock::now() + std::chrono::days {1};
       ++i)
  {
    decltype(cells)::value_type day_size {};
    while (current_day != relevant_days.end()
           && to_ymd(current_day->first) == std::chrono::year_month_day {i})
    {
      day_size += static_cast<std::uint32_t>(
          std::filesystem::file_size(current_day->second));
      ++entry_count;
      std::advance(current_day, 1);
    }
    cells.push_back(day_size);
  }
  assert(entry_count
         == static_cast<std::size_t>(std::ranges::distance(relevant_days)));
  return {entry_count, cells};
}

struct RGB
{
  unsigned char red;
  unsigned char green;
  unsigned char blue;
};

auto make_gradient(const RGB& colorA, const RGB& colorB, double factor)
{
  assert(factor >= 0);
  assert(factor <= 1);
  const auto red =
      static_cast<unsigned char>(std::lerp(colorA.red, colorB.red, factor));
  const auto green =
      static_cast<unsigned char>(std::lerp(colorA.green, colorB.green, factor));
  const auto blue =
      static_cast<unsigned char>(std::lerp(colorA.blue, colorB.blue, factor));

  return RGB {red, green, blue};
}

auto print_year(std::span<const std::uint32_t> cells, std::chrono::year year)
{
  const auto skip_first_day_count =
      (std::chrono::weekday {std::chrono::January / 01 / year}.iso_encoding()
       - 1);
  for (unsigned int i = 0; i < 7; ++i) {
    if (skip_first_day_count > i) {
      std::print("   ");
    }
    for (const auto d : cells
             | std::views::drop((7 - skip_first_day_count + i) % 7)
             | std::views::stride(7))
    {
      const auto chosen_color = [&]()
      {
        if (d == 0) {
          return RGB {0x5A, 0xD5, 0x2D};
        }
        return make_gradient(RGB {0x91, 0x9B, 0xFF},
                             RGB {0x13, 0x3A, 0x94},
                             static_cast<double>(d) / 20000.);
      }();
      std::print("\x1b[48;2;{};{};{}m{:2}\x1b[0m ",
                 chosen_color.red,
                 chosen_color.green,
                 chosen_color.blue,
                 d / 1000);
    }
    std::println("");
  }
}

void repo_stats(const repo_path_t& repo)
{
  const auto entries = list_entries(repo);
  if (entries.empty()) {
    std::println("No entries");
    return;
  }
  auto time = to_ymd(entries.front().first);
  std::size_t current_index = 0;
  auto year = time.year();
  while (current_index < entries.size()) {
    const auto& [index_delta, cells] =
        handle_year(std::ranges::subrange(
                        entries.begin() + static_cast<int64_t>(current_index),
                        entries.end()),
                    year);
    std::println("{}", year);
    print_year(cells, year);
    current_index += index_delta;
    ++year;
  }
}
