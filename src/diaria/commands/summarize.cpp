#include <chrono>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <format>
#include <fstream>
#include <ios>
#include <iterator>
#include <optional>
#include <print>
#include <ranges>
#include <stdexcept>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <vector>

#include "./summarize.hpp"

#include "crypto/entry.hpp"
#include "crypto/secret_key.hpp"
#include "diaria/repo_management.hpp"

// TODO: This is an N*m approach over a sorted list
// This is a place to optimize, but only once we are managing a billion entries
template<std::ranges::forward_range DatePairs, std::ranges::forward_range Dates>
  requires std::is_same_v<typename std::ranges::range_value_t<DatePairs>,
                          std::pair<time_point, time_point>>
auto find_indices_within_ranges(const DatePairs& ranges,
                                const Dates& dates) -> std::vector<std::size_t>
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

auto build_relevant_entry_list(
    const std::vector<std::pair<time_point, std::filesystem::path>>& list)
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
      | std::ranges::views::transform([](const auto& entry)
                                      { return entry.first; });
  auto relevant_entry_indices = find_indices_within_ranges(ranges, dates);
  auto relevant_entries = std::ranges::reverse_view {relevant_entry_indices}
      | std::ranges::views::transform([&list](const auto& index)
                                      { return list[index]; });
  return relevant_entries | std::ranges::to<std::vector>();
}

void print_entries(const key_repo_paths_t& keypath,
                   const std::ranges::forward_range auto& relevant_entries,
                   std::string_view password,
                   bool paging)
{
  const auto private_key =
      load_private_key(keypath.get_private_key_path(), password);
  const auto symkey = load_symkey(keypath.get_symkey_path());
  for (const auto& entry : relevant_entries) {
    std::ifstream stream(entry.second, std::ios::in | std::ios::binary);
    if (stream.fail()) {
      throw std::runtime_error("Could not open entry file");
    }
    std::vector<unsigned char> contents(
        (std::istreambuf_iterator<char>(stream)),
        std::istreambuf_iterator<char>());

    const auto decrypted = decrypt(
        symkey_span_t {symkey}, private_key_span_t {private_key}, contents);
    const std::string decrypted_decoded(decrypted.begin(), decrypted.end());
    if (paging) {
      std::print(
          "\x1b"
          "c");
    }
    std::println(
        "Reading entry from {:%F %H:%M}\n{}\n", entry.first, decrypted_decoded);
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

void summarize_repo(const key_repo_paths_t& keypath,
                    const repo_path_t& repo,
                    bool paging)
{
  const auto list = list_entries(repo);
  const auto relevant_entries = build_relevant_entry_list(list);
  std::println("Relevant entries: {}", relevant_entries.size());

  for (const auto& entry : relevant_entries) {
    const auto bytecount = std::filesystem::file_size(entry.second);
    std::println("\t{:%F %H:%M} - {: 5} bytes", entry.first, bytecount);
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

  const auto password =
      keypath.private_key_password
          .or_else([]() { return std::optional(read_password()); })
          .value();
  print_entries(keypath, relevant_entries, password, paging);
  if (paging) {
    std::print(
        "\x1b"
        "c");
  }
}