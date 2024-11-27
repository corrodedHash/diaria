#include <cstdlib>
#include <string>

#include <unistd.h>

auto read_password() -> std::string
{
  return {getpass("Enter password: ")};
}