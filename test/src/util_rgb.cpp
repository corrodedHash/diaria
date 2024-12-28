#include <catch2/catch_test_macros.hpp>

#include "util/rgb.hpp"

TEST_CASE("RGB class")
{
  RGB a {99, 100, 101};
  RGB red {255, 0, 0};
  RGB green {0, 255, 0};
  RGB blue {0, 0, 255};
  SECTION("reading a value")
  {
    REQUIRE(a.red == 99);
  }
  SECTION("green is perceived brighter than blue")
  {
    REQUIRE(green.luminance() > blue.luminance());
  }
}
