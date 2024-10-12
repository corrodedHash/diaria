#pragma once
#include <algorithm>
#include <print>

template<class R>
auto print_byte_range(R&& byte_range)
{
  const auto byte_printer = [](auto byte)
  { std::print(stderr, "{:02x}", byte); };
  std::ranges::for_each(byte_range, byte_printer);
}