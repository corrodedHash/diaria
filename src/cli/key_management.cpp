#include <cstdlib>

#include "./key_management.hpp"

#include <unistd.h>

auto read_password() -> safe_string
{
  return {getpass("Enter password: ")};
}