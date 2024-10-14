#pragma once
#include <cstdlib>
#include <span>  // For std::span
#include <stdexcept>

#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>  // For fstat
#include <unistd.h>

class mmap_file
{
public:
  // Constructor: Takes the file descriptor and maps the file into memory
  explicit mmap_file(int my_fd)
      : fd(my_fd)
  {
    // Get the size of the file
    struct stat file_stat
    {
    };
    if (fstat(fd, &file_stat) == -1) {
      close(fd);
      throw std::runtime_error("Failed to get file size");
    }
    size = static_cast<std::size_t>(file_stat.st_size);

    // Map the file into memory
    mapped_region = mmap(nullptr, size, PROT_READ, MAP_PRIVATE, fd, 0);
    if (mapped_region == MAP_FAILED) {
      close(fd);
      throw std::runtime_error("Failed to mmap file");
    }

    // Initialize the span
    span = std::span<const unsigned char>(
        static_cast<unsigned char*>(mapped_region), size);
  }

  // Delete copy constructor
  mmap_file(const mmap_file&) = delete;

  // Delete copy assignment operator
  auto operator=(const mmap_file&) -> mmap_file& = delete;

  // Move constructor
  mmap_file(mmap_file&& other) noexcept
      : fd(other.fd)
      , mapped_region(other.mapped_region)
      , size(other.size)
      , span(other.span)
  {
    // Invalidate the moved-from object
    other.fd = -1;
    other.mapped_region = MAP_FAILED;
    other.size = 0;
    other.span = {};
  }

  // Move assignment operator
  auto operator=(mmap_file&& other) noexcept -> mmap_file&
  {
    if (this != &other) {
      // Clean up any existing resources
      if (mapped_region != MAP_FAILED) {
        munmap(mapped_region, size);
      }
      if (fd != -1) {
        close(fd);
      }

      // Move resources from the other object
      fd = other.fd;
      mapped_region = other.mapped_region;
      size = other.size;
      span = other.span;

      // Invalidate the moved-from object
      other.fd = -1;
      other.mapped_region = MAP_FAILED;
      other.size = 0;
      other.span = {};
    }
    return *this;
  }

  // Destructor: Unmap the memory and close the file descriptor
  ~mmap_file()
  {
    if (mapped_region != MAP_FAILED) {
      munmap(mapped_region, size);
    }
    if (fd != -1) {
      close(fd);
    }
  }

  // Get the span representing the file contents
  [[nodiscard]] auto get_span() const -> std::span<const unsigned char>
  {
    return span;
  }

private:
  int fd;  // File descriptor
  void* mapped_region;  // Pointer to the memory-mapped region
  size_t size;  // Size of the file
  std::span<const unsigned char> span;  // Span over the mapped memory
};