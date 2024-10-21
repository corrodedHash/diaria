#include <algorithm>
#include <chrono>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <optional>
#include <print>
#include <ranges>
#include <spanstream>
#include <stdexcept>
#include <type_traits>
#include <utility>

#include <bits/chrono.h>

#include "../repo.hpp"
#include "crypto/entry.hpp"

using time_point = std::chrono::utc_clock::time_point;

// Function to parse the timestamp from the filename
auto parse_timestamp(std::string_view filename) -> std::optional<time_point>
{
  std::ispanstream input_stream {filename};
  time_point timepoint;
  std::chrono::from_stream(input_stream, "%FT%T", timepoint);
  if (input_stream.fail()) {
    throw std::runtime_error(std::format("Could not parse: \"{}\"", filename));
  }
  return timepoint;
}

auto list_entries(const repo_path_t& repo)
    -> std::vector<std::pair<time_point, std::filesystem::path>>
{
  auto result =
      std::filesystem::directory_iterator(repo.repo)
      | std::views::filter([](auto entry) { return entry.is_regular_file(); })
      | std::views::filter(
          [](auto entry)
          { return entry.path().filename().string().ends_with(".diaria"); })
      | std::views::transform(
          [](const auto& entry)
          {
            return std::make_pair(
                parse_timestamp(std::string(entry.path().filename())),
                entry.path());
          })
      | std::views::filter([](const auto& entry)
                           { return entry.first.has_value(); })
      | std::views::transform(
          [](auto&& entry)
          { return std::make_pair(entry.first.value(), entry.second); })
      | std::ranges::to<std::vector>();
  std::ranges::sort(result, {}, [](const auto& entry) { return entry.first; });
  return result;
}

// TODO: This is an N*m approach over a sorted list
// This is a place to optimize, but only once we are managing a billion entries
template<std::ranges::forward_range DatePairs, std::ranges::forward_range Dates>
  requires std::is_same_v<typename DatePairs::value_type,
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
            return std::make_pair(
                timepoint_now - date_range.first - date_range.second,
                timepoint_now - date_range.first + date_range.second);
          })
      | std::ranges::to<std::vector>();
  auto relevant_entry_indices = find_indices_within_ranges(
      ranges,
      list
          | std::ranges::views::transform([](const auto& entry)
                                          { return entry.first; }));
  auto relevant_entries = std::ranges::reverse_view {relevant_entry_indices}
      | std::ranges::views::transform([&list](const auto& index)
                                      { return list[index]; });
  return relevant_entries | std::ranges::to<std::vector>();
}

void print_entries(const key_repo_t& keypath,
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
    std::println("Reading entry from {:%F %HH:%MM}\n{}\n",
                 entry.first,
                 decrypted_decoded);
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

void summarize_repo(const key_repo_t& keypath,
                    const repo_path_t& repo,
                    std::string_view password,
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

  print_entries(keypath, relevant_entries, password, paging);
  if (paging) {
    std::print(
        "\x1b"
        "c");
  }
}