#include <chrono>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <format>
#include <fstream>
#include <ios>
#include <iterator>
#include <print>
#include <ranges>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

#include "./summarize.hpp"

#include "cli/key_management.hpp"
#include "cli/repo_management.hpp"

namespace
{
// TODO: This is an N*m approach over a sorted list
// This is a place to optimize, but only once we are managing a billion entries
template<std::ranges::forward_range DatePairs, std::ranges::forward_range Dates>
  requires std::is_same_v<typename std::ranges::range_value_t<DatePairs>,
                          std::pair<time_point, time_point>>
auto find_indices_within_ranges(const DatePairs& ranges, const Dates& dates)
    -> std::vector<std::size_t>
{
  std::vector<std::size_t> indices;

  // Iterate over all dates
  for (const auto& [date_index, date] : dates | std::ranges::views::enumerate) {
    // Check if the date falls within any of the ranges
    for (const auto& range : ranges) {
      if (date >= range.first && date <= range.second) {
        indices.push_back(static_cast<std::size_t>(
            date_index));  // Save the index of the matching date
        break;  // Stop checking further ranges for this date
      }
    }
  }

  return indices;
}

auto build_relevant_entry_list(const std::vector<diaria_entry_path>& list)
{
  const auto half_day = std::chrono::hours(12);
  const auto ranges_delta =
      std::vector<std::pair<std::chrono::duration<std::int64_t>,
                            std::chrono::duration<std::int64_t>>> {
          {std::chrono::days(1), half_day},
          {std::chrono::weeks(1), half_day},
          {std::chrono::months(1), half_day},
          {std::chrono::years(1), half_day},
          {std::chrono::years(2), half_day},
          {std::chrono::years(4), half_day},
          {std::chrono::years(8), half_day},
          {std::chrono::years(16), half_day}};
  const auto timepoint_now = std::chrono::utc_clock::now();
  const auto ranges =
      ranges_delta
      | std::ranges::views::transform(
          [&timepoint_now](auto& date_range)
          {
            auto& [distance, delta] = date_range;
            return std::make_pair(timepoint_now - distance - delta,
                                  timepoint_now - distance + delta);
          });
  const auto dates = list
      | std::ranges::views::transform([](const diaria_entry_path& entry)
                                      { return entry.entry_time; });
  auto relevant_entry_indices = find_indices_within_ranges(ranges, dates);
  auto relevant_entries = std::ranges::reverse_view {relevant_entry_indices}
      | std::ranges::views::transform([&list](const auto& index)
                                      { return list[index]; });
  return relevant_entries | std::ranges::to<std::vector>();
}

void print_entries(entry_decryptor decryptor,
                   const std::ranges::forward_range auto& relevant_entries,
                   bool paging)
{
  for (const auto& entry : relevant_entries) {
    std::ifstream stream(entry.entry_path, std::ios::in | std::ios::binary);
    if (stream.fail()) {
      throw std::runtime_error("Could not open entry file");
    }
    std::vector<unsigned char> contents(
        (std::istreambuf_iterator<char>(stream)),
        std::istreambuf_iterator<char>());

    const auto decrypted = decryptor.decrypt(contents);
    const std::string decrypted_decoded(decrypted.begin(), decrypted.end());
    if (paging) {
      std::print(
          "\x1b"
          "c");
    }
    std::println("Reading entry from {:%F %H:%M}", entry.entry_time);
    std::println("{}", decrypted_decoded);
    std::println();
    if (paging) {
      std::println("Press [Enter] for next entry");
      if (std::getchar() == EOF) {
        std::println(
            "\x1b"
            "c"
            "End of file");
        return;
      }
    }
  }
}
}  // namespace

void summarize_repo(std::unique_ptr<entry_decryptor_initializer> keys,
                    const repo_path_t& repo,
                    bool paging)
{
  const auto list = list_entries(repo);
  const auto relevant_entries = build_relevant_entry_list(list);
  std::println("Relevant entries: {}", relevant_entries.size());

  for (const auto& entry : relevant_entries) {
    const auto bytecount = std::filesystem::file_size(entry.entry_path);
    std::println("\t{:%F %H:%M} - {: 5} bytes", entry.entry_time, bytecount);
  }
  if (paging) {
    std::println("Press [Enter] to start");
    if (std::getchar() == EOF) {
      std::println(
          "\x1b"
          "c"
          "End of file");
      return;
    }
  }

  print_entries(keys->init(), relevant_entries, paging);
  if (paging) {
    std::print(
        "\x1b"
        "c");
  }
}