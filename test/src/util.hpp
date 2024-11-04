#pragma once
#include <algorithm>
#include <print>

#include <catch2/matchers/catch_matchers_templated.hpp>

template<typename Range>
struct equals_range_matcher : Catch::Matchers::MatcherGenericBase
{
  explicit equals_range_matcher(Range const& input_range)
      : range {&input_range}
  {
  }

  template<typename OtherRange>
  auto match(OtherRange const& other) const -> bool
  {
    using std::begin;
    using std::end;

    return std::equal(begin(*range), end(*range), begin(other), end(other));
  }

  auto describe() const -> std::string override
  {
    return "Equals: " + Catch::rangeToString(*range);
  }

private:
  const Range* range;
};

template<typename Range>
auto equals_range(const Range& range) -> equals_range_matcher<Range>
{
  return equals_range_matcher<Range> {range};
}

template<class R>
auto print_byte_range(const R&& byte_range)
{
  const auto byte_printer = [](auto byte)
  { std::print(stderr, "{:02x}", byte); };
  std::ranges::for_each(byte_range, byte_printer);
}