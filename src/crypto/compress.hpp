
#include <cstdbool>
#include <cstdlib>
#include <print>
#include <vector>

#include <lzma.h>

auto compress(std::span<const unsigned char> input) -> std::vector<unsigned char>;

auto decompress(std::span<const unsigned char> input)
    -> std::vector<unsigned char>;
