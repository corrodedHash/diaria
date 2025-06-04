
#include "crypto/safe_buffer.hpp"

auto compress(std::span<const unsigned char> input)
    -> safe_vector<unsigned char>;

auto decompress(std::span<const unsigned char> input)
    -> safe_vector<unsigned char>;
