#include <cassert>
#include <chrono>
#include <cstdint>
#include <filesystem>
#include <optional>
#include <print>
#include <ranges>
#include <span>

#include "./stats.hpp"

#include <bits/chrono.h>
#include <bits/ranges_algo.h>
#include <sys/types.h>

#include "cli/repo_management.hpp"
#include "util/rgb.hpp"
#include "util/time.hpp"

namespace
{
/**
@param soft_max_day When no further days containing entries exist, cut entry
generation off at this date
@return A vector containing the file size of all entries made on each day within
the year
 */
auto handle_year(const std::ranges::forward_range auto& entries,
                 std::chrono::year year,
                 std::optional<std::chrono::year_month_day> soft_max_day)
    -> std::vector<std::uint64_t>
{
  auto day_entry_chunks = entries
      | std::views::chunk_by(
                              [](const diaria_entry_path& last_entry,
                                 const diaria_entry_path& this_entry)
                              {
                                return to_ymd(last_entry.entry_time)
                                    == to_ymd(this_entry.entry_time);
                              });
  auto current_day_entry_chunk = day_entry_chunks.begin();
  auto current_day_entry_chunk_end = day_entry_chunks.end();

  const auto cells =
      days_of_year(year)
      | std::views::transform([](const auto& day)
                              { return std::chrono::year_month_day {day}; })
      | std::views::take_while(
          [&soft_max_day,
           &current_day_entry_chunk,
           &current_day_entry_chunk_end](const auto& day)
          {
            return !soft_max_day.has_value()
                || (current_day_entry_chunk != current_day_entry_chunk_end
                    || day < soft_max_day.value());
          })
      | std::views::transform(
          [&current_day_entry_chunk,
           &current_day_entry_chunk_end](const auto& current_day)
          {
            if (current_day_entry_chunk == current_day_entry_chunk_end
                || to_ymd((*current_day_entry_chunk).begin()->entry_time)
                    != current_day)
            {
              return 0UL;
            }

            auto current_day_entry_sizes = *current_day_entry_chunk
                | std::views::transform(
                    [](const auto& entry)
                    { return (std::filesystem::file_size(entry.entry_path)); });
            const auto size_sum = std::ranges::fold_left(
                current_day_entry_sizes, 0, std::plus {});

            ++current_day_entry_chunk;
            return size_sum;
          })
      | std::ranges::to<std::vector>();
  return cells;
}

auto print_year(std::span<const std::uint64_t> cells, std::chrono::year year)
    -> std::string
{
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

  const auto first_weekday =
      (std::chrono::weekday {std::chrono::January / 01 / year}.iso_encoding()
       - 1);

  constexpr int days_in_week = 7;
  std::string result {};

  const auto print_weekday_line = [&](auto weekday)
  {
    const auto days_until_weekday = (7 - first_weekday + weekday) % 7;
    const auto lines =
        cells | std::views::drop(days_until_weekday)
        | std::views::stride(days_in_week)
        | std::views::transform(
            [&](const auto cell_byte_count)
            {
              const auto chosen_color =
                  map_color_range(byte_gradient_mapping, cell_byte_count);
              const auto foreground_color =
                  chosen_color.luminance() < 140 ? color_white : color_black;
              constexpr int bytes_per_kilobyte = 1000;
              constexpr std::string_view ansi_clear_style {"\x1b[0m"};
              return std::format("{}{}{:2}{} ",
                                 chosen_color.ansi_24_back(),
                                 foreground_color.ansi_24_fore(),
                                 cell_byte_count / bytes_per_kilobyte,
                                 ansi_clear_style);
            });

    return std::ranges::fold_left(lines, std::string(), std::plus {});
  };

  for (unsigned int i = 0; i < days_in_week; ++i) {
    if (first_weekday > i) {
      result += std::string(3, ' ');
    }
    result += print_weekday_line(i);
    result += '\n';
  }
  return result;
}

auto print_year_header(std::chrono::year year) -> std::string
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

  constexpr unsigned int weekwidth = 3;
  std::string line {};
  for (const auto [index, month] : std::views::enumerate(month_names)) {
    auto month_start = std::chrono::year_month_day {
        year,
        std::chrono::month {static_cast<unsigned int>((index + 1))},
        std::chrono::day {1}};

    const auto week = calendar_week(month_start);
    const auto output_width = week * weekwidth;
    if (output_width < line.length()) {
      continue;
    }
    line += std::string(output_width - line.length(), ' ');
    line += month;
  }
  return std::format("{}{}\n{}\n", std::string(4, ' '), year, line);
}
}  // namespace

void repo_stats(const repo_path_t& repo)
{
  const auto entries = list_entries(repo);
  if (entries.empty()) {
    std::println("No entries");
    return;
  }
  auto entries_by_year = entries
      | std::views::chunk_by(
                             [](const diaria_entry_path& last_entry,
                                const diaria_entry_path& current_entry)
                             {
                               return to_ymd(last_entry.entry_time).year()
                                   == to_ymd(current_entry.entry_time).year();
                             });
  const auto now = std::chrono::system_clock::now();
  const auto ymd4 =
      std::chrono::year_month_day(std::chrono::floor<std::chrono::days>(now));
  // TODO: Will have weird behavior if entries are from future years
  for (auto const entry_year : entries_by_year) {
    if (entry_year.empty()) {
      continue;
    }
    const auto year = to_ymd(entry_year.begin()->entry_time).year();
    const auto cells = handle_year(entry_year, year, std::optional(ymd4));
    std::print("{}", print_year_header(year));
    std::print("{}", print_year(cells, year));
  }
}
