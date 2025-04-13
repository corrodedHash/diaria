#include <catch2/catch_test_macros.hpp>

#include "util/rgb.hpp"

namespace Catch
{
template<>
struct StringMaker<RGB>
{
  static auto convert(RGB const& value) -> std::string
  {
    return std::format(
        "{{R: {} G: {} B: {}}}", value.red, value.green, value.blue);
  }
};
}  // namespace Catch

TEST_CASE("RGB class")
{
  constexpr RGB gray {.red = 99, .green = 100, .blue = 101};
  constexpr RGB red {.red = 255, .green = 0, .blue = 0};
  constexpr RGB green {.red = 0, .green = 255, .blue = 0};
  constexpr RGB blue {.red = 0, .green = 0, .blue = 255};
  SECTION("constructing from hex value")
  {
    REQUIRE(RGB::from_hex(0x636465) == gray);
  }
  SECTION("reading a value")
  {
    REQUIRE(gray.red == 99);
  }
  SECTION("green is perceived brighter than blue")
  {
    REQUIRE(green.luminance() > blue.luminance());
  }
  SECTION("gradient")
  {
    const auto red_blue = make_gradient(red, blue, 0.5);
    REQUIRE(red_blue.green == 0);
    REQUIRE(red_blue.red > 0);
    REQUIRE(red_blue.blue > 0);

    const auto gradient_blue = make_gradient(red, blue, 1.);
    REQUIRE(gradient_blue == blue);
  }

  const std::array<std::pair<uint32_t, RGB>, 4> gradient_mapping = {
      {{0, gray}, {0, red}, {500, green}, {4000, blue}}};

  SECTION("mapping gradient thresholds")
  {
    REQUIRE(map_color_range(gradient_mapping, 0) == gray);
    REQUIRE(map_color_range(gradient_mapping, 500) == green);
    REQUIRE(map_color_range(gradient_mapping, 4000) == blue);
    REQUIRE(map_color_range(gradient_mapping, 10000) == blue);
  }
  SECTION("gradient on RGB")
  {
    const auto green_blue = map_color_range(gradient_mapping, 2000);
    REQUIRE(green_blue.red == 0);
    REQUIRE(green_blue.green > 0);
    REQUIRE(green_blue.blue > 0);
  }
}
