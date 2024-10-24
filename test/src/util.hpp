#pragma once
#include <algorithm>
#include <print>

#include <catch2/matchers/catch_matchers_templated.hpp>

template<typename Range>
struct EqualsRangeMatcher : Catch::Matchers::MatcherGenericBase
{
  EqualsRangeMatcher(Range const& range)
      : range {range}
  {
  }

  template<typename OtherRange>
  auto match(OtherRange const& other) const -> bool
  {
    using std::begin;
    using std::end;

    return std::equal(begin(range), end(range), begin(other), end(other));
  }

  auto describe() const -> std::string override
  {
    return "Equals: " + Catch::rangeToString(range);
  }

private:
  Range const& range;
};

template<typename Range>
auto EqualsRange(const Range& range) -> EqualsRangeMatcher<Range>
{
  return EqualsRangeMatcher<Range> {range};
}

template<class R>
auto print_byte_range(R&& byte_range)
{
  const auto byte_printer = [](auto byte)
  { std::print(stderr, "{:02x}", byte); };
  std::ranges::for_each(byte_range, byte_printer);
}