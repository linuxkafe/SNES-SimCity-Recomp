#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>
#include <string>

namespace snes {

class SnesRom;

enum class DecompressError {
    None,
    Truncated,
    BadControl,
    BadRef,
    BufferOverflow,
    EndNotFound
};

std::string decompress_error_name(DecompressError e);

// Decompress a Nintendo packet stream from ROM at given offset.
// On success: returns true, out contains decompressed bytes, end_offset is byte after 0xFF.
// On failure: returns false, out is cleared, end_offset is offset of failure.
bool nintendo_decompress(const SnesRom& rom, size_t offset,
                         std::vector<uint8_t>& out, size_t& end_offset,
                         DecompressError* error_out = nullptr);

} // namespace snes