#pragma once
// NOLINTBEGIN(cppcoreguidelines-pro-type-reinterpret-cast)
inline auto make_unsigned_char(char* input) -> unsigned char*
{
  return reinterpret_cast<unsigned char*>(input);
}

inline auto make_unsigned_char(const char* input) -> const unsigned char*
{
  return reinterpret_cast<const unsigned char*>(input);
}

inline auto make_signed_char(unsigned char* input) -> char*
{
  return reinterpret_cast<char*>(input);
}
inline auto make_signed_char(const unsigned char* input) -> const char*
{
  return reinterpret_cast<const char*>(input);
}
// NOLINTEND(cppcoreguidelines-pro-type-reinterpret-cast)