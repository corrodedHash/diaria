#pragma once

#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstdint>
#include <iterator>
#include <print>
#include <ranges>

// NOLINTNEXTLINE(readability-identifier-naming)
struct RGB
{
  unsigned char red {};
  unsigned char green {};
  unsigned char blue {};
  constexpr static auto from_hex(uint32_t hexcode) -> RGB
  {
    constexpr uint32_t byte_bitmask = 0xff;
    constexpr uint32_t channel_width = 8;
    return RGB {
        static_cast<unsigned char>((hexcode >> (2 * (channel_width)))
                                   & byte_bitmask),
        static_cast<unsigned char>((hexcode >> channel_width) & byte_bitmask),
        static_cast<unsigned char>(hexcode & byte_bitmask)};
  }
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

inline auto make_gradient(const RGB& color_a, const RGB& color_b, double factor)
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

template<typename T>
// requires(
//     std::is_same_v<decltype(std::get<0>typename
//     std::ranges::range_value_t<ThresholdRange>, T>, std::is_same_v<typename
//     std::ranges::range_value_t<ColorRange>, RGB>)

auto map_color_range(const std::ranges::forward_range auto& threshold_colors,
                     T number)
{
  auto const higher_color = std::ranges::partition_point(
      threshold_colors,
      [number](auto color_step) { return number > std::get<0>(color_step); });
  if (higher_color == threshold_colors.begin()) {
    return std::get<1>(*higher_color);
  }
  auto const lower_color = std::prev(higher_color);
  if (higher_color == threshold_colors.end()) {
    return std::get<1>(*lower_color);
  }
  const auto ratio = static_cast<double>(number - std::get<0>(*lower_color))
      / static_cast<double>(std::get<0>(*higher_color)
                            - std::get<0>(*lower_color));
  return make_gradient(
      std::get<1>(*lower_color), std::get<1>(*higher_color), ratio);
}
