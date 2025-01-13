#include <algorithm>
#include <ranges>
#include <spanstream>
#include <vector>

#include "repo_management.hpp"

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

// Returns the diaria entries, sorted by their creation date
auto list_entries(const repo_path_t& repo) -> std::vector<diaria_entry_path>
{
  auto result =
      std::filesystem::directory_iterator(repo.repo)
      | std::views::filter([](const auto& entry)
                           { return entry.is_regular_file(); })
      | std::views::filter(
          [](const auto& entry)
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
          { return diaria_entry_path {entry.first.value(), entry.second}; })
      | std::ranges::to<std::vector>();
  std::ranges::sort(result,
                    {},
                    [](const diaria_entry_path& entry)
                    { return entry.entry_time; });
  return result;
}