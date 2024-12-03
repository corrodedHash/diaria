#include <cassert>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <filesystem>
#include <print>
#include <ranges>
#include <span>

#include "./stats.hpp"

#include <bits/chrono.h>
#include <bits/ranges_algo.h>
#include <sys/types.h>

#include "diaria/repo_management.hpp"
#include "util/rgb.hpp"

auto to_ymd(const std::chrono::utc_clock::time_point& input_time)
{
  return std::chrono::year_month_day {std::chrono::floor<std::chrono::days>(
      std::chrono::system_clock::time_point((input_time.time_since_epoch())))};
}
auto calendar_week(const std::chrono::year_month_day& day) -> unsigned int
{
  constexpr int days_in_week = 7;
  // Number of days in the first week of the year
  std::chrono::days second_week_delta {
      days_in_week
      - (std::chrono::weekday {std::chrono::January / 01 / day.year()}
             .iso_encoding()
         - 1)};
  std::chrono::year_month_day second_week_start {
      std::chrono::January / static_cast<int>(1 + second_week_delta.count())
      / day.year()};
  if (second_week_start > day) {
    return 0;
  }
  std::chrono::days day_delta =
      std::chrono::sys_days(day) - std::chrono::sys_days(second_week_start);
  return static_cast<unsigned int>(day_delta.count() / days_in_week) + 1;
}

auto days_of_year(std::chrono::year year)
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

auto handle_year(const std::ranges::forward_range auto& entries,
                 std::chrono::year year) -> std::vector<std::uint32_t>
{
  auto day_entry_chunks =
      entries
      | std::views::chunk_by(
          [](auto last_entry, auto this_entry)
          { return to_ymd(last_entry.first) == to_ymd(this_entry.first); });
  auto current_day_entry_chunk = day_entry_chunks.begin();

  constexpr std::size_t days_in_year = 366;
  std::vector<std::uint32_t> cells {};
  cells.reserve(days_in_year);

  for (auto const current_day : days_of_year(year)) {
    if (current_day_entry_chunk == day_entry_chunks.end()
        || to_ymd((*current_day_entry_chunk).begin()->first)
            != std::chrono::year_month_day {current_day})
    {
      cells.push_back(0);
    } else {
      auto current_day_entry_sizes = *current_day_entry_chunk
          | std::views::transform(
              [](auto& entry)
              {
                return static_cast<std::uint32_t>(
                    std::filesystem::file_size(entry.second));
              });

      cells.push_back(
          std::ranges::fold_left(current_day_entry_sizes, 0, std::plus {}));
      ++current_day_entry_chunk;
    }
  }
  return cells;
}

auto print_year(std::span<const std::uint32_t> cells, std::chrono::year year)
{
  const auto skip_first_day_count =
      (std::chrono::weekday {std::chrono::January / 01 / year}.iso_encoding()
       - 1);
  constexpr RGB color_atlantis = RGB::from_hex(0x5ad52d);

  constexpr RGB color_titan_white = RGB::from_hex(0xe5e8ff);
  constexpr RGB color_melrose = RGB::from_hex(0x919bff);
  constexpr RGB color_torea_bay = RGB::from_hex(0x133a94);
  constexpr RGB color_wild_strawberry = RGB::from_hex(0xff407e);

  constexpr RGB color_black = RGB::from_hex(0x000000);
  constexpr RGB color_white = RGB::from_hex(0xFFFFFF);

  constexpr std::array<std::pair<uint32_t, RGB>, 5> byte_gradient_mapping = {
      {{0, color_atlantis},
       {0, color_titan_white},
       {500, color_melrose},
       {4000, color_torea_bay},
       {12000, color_wild_strawberry}}};

  constexpr int days_in_week = 7;
  for (unsigned int i = 0; i < days_in_week; ++i) {
    if (skip_first_day_count > i) {
      std::print("   ");
    }
    for (const auto cell_byte_count : cells
             | std::views::drop((7 - skip_first_day_count + i) % 7)
             | std::views::stride(7))
    {
      const auto chosen_color =
          map_color_range(byte_gradient_mapping, cell_byte_count);
      const auto foreground_color =
          chosen_color.luminance() < 140 ? color_white : color_black;
      const auto bytes_per_kilobyte = 1000;
      std::print("\x1b[48;2;{};{};{}m\x1b[38;2;{};{};{}m{:2}\x1b[0m ",
                 chosen_color.red,
                 chosen_color.green,
                 chosen_color.blue,
                 foreground_color.red,
                 foreground_color.green,
                 foreground_color.blue,
                 cell_byte_count / bytes_per_kilobyte);
    }
    std::println("");
  }
}

auto print_year_header(std::chrono::year year)
{
  constexpr std::array<std::string_view, 12> month_names = {"Jan",
                                                            "Feb",
                                                            "Mar",
                                                            "Apr",
                                                            "May",
                                                            "Jun",
                                                            "Jul",
                                                            "Aug",
                                                            "Sep",
                                                            "Oct",
                                                            "Nov",
                                                            "Dec"};

  auto index = 0;
  constexpr int weekwidth = 3;
  std::println("    {}", year);
  std::string line{};
  for (const auto month : month_names) {
    auto month_start = std::chrono::year_month_day {
        year,
        std::chrono::month {static_cast<unsigned int>(index + 1)},
        std::chrono::day {1}};
    index += 1;

    auto week = calendar_week(month_start);
    if (week * weekwidth < line.length()) {
      continue;
    }
    line += std::string(week * weekwidth - line.length(), ' ');
    line += month;
  }
  std::println("{}", line);
}

void repo_stats(const repo_path_t& repo)
{
  const auto entries = list_entries(repo);
  if (entries.empty()) {
    std::println("No entries");
    return;
  }
  auto entries_by_year =
      entries
      | std::views::chunk_by(
          [](const auto& last_entry, const auto& current_entry)
          {
            return to_ymd(last_entry.first).year()
                == to_ymd(current_entry.first).year();
          });
  for (auto const entry_year : entries_by_year) {
    if (entry_year.empty()) {
      continue;
    }
    const auto year = to_ymd(entry_year.begin()->first).year();
    const auto cells = handle_year(entry_year, year);
    print_year_header(year);
    print_year(cells, year);
  }
}
