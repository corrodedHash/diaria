#include <cstdlib>

#include <unistd.h>

#include "crypto/secret_key.hpp"

auto read_password() -> safe_string
{
  return {getpass("Enter password: ")};
}