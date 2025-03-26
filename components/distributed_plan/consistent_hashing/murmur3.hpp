#pragma once

#include <cstdint>

namespace consistent_hashing {
    uint64_t murmurhash3_x64_128(const void* key, int len, uint64_t seed);
} // namespace consistent_hashing