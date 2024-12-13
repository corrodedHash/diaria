#pragma once
#include <unistd.h>
struct smart_fd
{
  explicit smart_fd(int new_fd)
      : fd(new_fd)
  {
  }
  smart_fd(const smart_fd&) = delete;
  smart_fd(smart_fd&&) = delete;
  auto operator=(const smart_fd&) -> smart_fd& = delete;
  auto operator=(smart_fd&&) -> smart_fd& = delete;
  int fd;
  ~smart_fd() { close(fd); }
};