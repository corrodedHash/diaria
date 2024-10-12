#pragma once
#include <cstdlib>
#include <print>
#include <span>  // For std::span
#include <stdexcept>

#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>  // For fstat
#include <sys/wait.h>
#include <unistd.h>


class mmap_file
{
public:
  // Constructor: Takes the file descriptor and maps the file into memory
  mmap_file(int fd)
      : fd(fd)
  {
    // Get the size of the file
    struct stat file_stat
    {
    };
    if (fstat(fd, &file_stat) == -1) {
      close(fd);
      throw std::runtime_error("Failed to get file size");
    }
    size = file_stat.st_size;

    // Map the file into memory
    mapped_region = mmap(nullptr, size, PROT_READ, MAP_PRIVATE, fd, 0);
    if (mapped_region == MAP_FAILED) {
      close(fd);
      throw std::runtime_error("Failed to mmap file");
    }

    // Initialize the span
    span = std::span<const unsigned char>(
        reinterpret_cast<const unsigned char*>(mapped_region), size);
  }

  // Delete copy constructor
  mmap_file(const mmap_file&) = delete;

  // Delete copy assignment operator
  mmap_file& operator=(const mmap_file&) = delete;

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
  mmap_file& operator=(mmap_file&& other) noexcept
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
  std::span<const unsigned char> getSpan() const { return span; }

private:
  int fd;  // File descriptor
  void* mapped_region;  // Pointer to the memory-mapped region
  size_t size;  // Size of the file
  std::span<const unsigned char> span;  // Span over the mapped memory
};