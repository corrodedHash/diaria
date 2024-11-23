#include <cassert>
#include <chrono>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <print>
#include <ranges>
#include <span>

#include "./stats.hpp"

#include <sys/types.h>
#include "diaria/repo_management.hpp"

auto to_ymd(const auto& input_time)
{
  return std::chrono::year_month_day {std::chrono::floor<std::chrono::days>(
      std::chrono::system_clock::time_point((input_time.time_since_epoch())))};
}

auto handle_year(const std::ranges::forward_range auto& entries,
                 std::chrono::year year)
    -> std::pair<std::size_t, std::vector<std::uint32_t>>
{
  auto relevant_days = entries
      | std::views::drop_while(
                           [&](const auto& entry_date)
                           { return to_ymd(entry_date.first).year() < year; })
      | std::ranges::views::take_while(
                           [&](const auto& entry_date)
                           { return to_ymd(entry_date.first).year() == year; });
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

// NOLINTNEXTLINE(readability-identifier-naming)
struct RGB
{
  unsigned char red;
  unsigned char green;
  unsigned char blue;
  [[nodiscard]] auto luminance() const -> double
  {
    // These are magic numbers I got online, just accept it
    // NOLINTBEGIN(cppcoreguidelines-avoid-magic-numbers,
    // readability-magic-numbers)
    return (0.2126 * red + 0.7152 * green + 0.0722 * blue);
    // NOLINTEND(cppcoreguidelines-avoid-magic-numbers,
    // readability-magic-numbers)
  }
};

auto make_gradient(const RGB& color_a, const RGB& color_b, double factor)
{
  assert(factor >= 0);
  assert(factor <= 1);
  const auto red =
      static_cast<unsigned char>(std::lerp(color_a.red, color_b.red, factor));
  const auto green = static_cast<unsigned char>(
      std::lerp(color_a.green, color_b.green, factor));
  const auto blue =
      static_cast<unsigned char>(std::lerp(color_a.blue, color_b.blue, factor));

  return RGB {red, green, blue};
}

auto print_year(std::span<const std::uint32_t> cells, std::chrono::year year)
{
  const auto skip_first_day_count =
      (std::chrono::weekday {std::chrono::January / 01 / year}.iso_encoding()
       - 1);
  constexpr RGB color_atlantis = RGB {0x5A, 0XD5, 0x2D};
  constexpr RGB color_melrose = RGB {0x91, 0x9B, 0xFF};
  constexpr RGB color_torea_bay = RGB {0x13, 0x3A, 0x94};
  constexpr RGB color_black = RGB {0x0, 0x0, 0x0};
  constexpr RGB color_white = RGB {0xFF, 0xFF, 0xFF};

  constexpr int days_in_week = 7;
  for (unsigned int i = 0; i < days_in_week; ++i) {
    if (skip_first_day_count > i) {
      std::print("   ");
    }
    for (const auto cell_byte_count : cells
             | std::views::drop((7 - skip_first_day_count + i) % 7)
             | std::views::stride(7))
    {
      const auto chosen_color = [&]()
      {
        if (cell_byte_count == 0) {
          return color_atlantis;
        }
        constexpr double max_bytes = 20000.;
        return make_gradient(color_melrose,
                             color_torea_bay,
                             static_cast<double>(cell_byte_count) / max_bytes);
      }();
      const auto foreground_color =
          chosen_color.luminance() < 140 ? color_white : color_black;
      std::print("\x1b[48;2;{};{};{}m\x1b[38;2;{};{};{}m{:2}\x1b[0m ",
                 chosen_color.red,
                 chosen_color.green,
                 chosen_color.blue,
                 foreground_color.red,
                 foreground_color.green,
                 foreground_color.blue,
                 cell_byte_count / 1000);
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
