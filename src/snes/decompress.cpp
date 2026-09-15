#include "snes/decompress.h"
#include "snes/rom.h"

#include <cstdint>
#include <string>
#include <vector>

namespace snes {

std::string decompress_error_name(DecompressError e) {
    switch (e) {
        case DecompressError::None: return "None";
        case DecompressError::Truncated: return "Truncated";
        case DecompressError::BadControl: return "BadControl";
        case DecompressError::BadRef: return "BadRef";
        case DecompressError::BufferOverflow: return "BufferOverflow";
        case DecompressError::EndNotFound: return "EndNotFound";
    }
    return "Unknown";
}

namespace {

inline bool read_byte_raw(const std::vector<uint8_t>& data, size_t& off, uint8_t& out) {
    if (off >= data.size()) return false;
    out = data[off];
    ++off;
    return true;
}

inline bool read_u16_raw(const std::vector<uint8_t>& data, size_t& off, uint16_t& out) {
    if (off + 1 >= data.size()) return false;
    out = static_cast<uint16_t>(data[off] | (data[off + 1] << 8));
    off += 2;
    return true;
}

} // namespace

bool nintendo_decompress(const SnesRom& rom, size_t offset,
                         std::vector<uint8_t>& out, size_t& end_offset,
                         DecompressError* error_out) {
    out.clear();
    end_offset = offset;
    DecompressError last_error = DecompressError::None;

    // Asset packets are located by raw file offset (see AssetPointersAndFiles.asm).
    // read8()/read16() expect CPU addresses, so index the raw bytes directly.
    const std::vector<uint8_t>& data = rom.data();

    while (true) {
        uint8_t ctrl;
        if (!read_byte_raw(data, offset, ctrl)) {
            last_error = DecompressError::Truncated;
            break;
        }
        if (ctrl == 0xFF) {
            end_offset = offset;
            if (error_out) *error_out = DecompressError::None;
            return true;
        }

        uint8_t mode = ctrl & 0xE0;
        unsigned length = ctrl & 0x1F;

        if (mode == 0xE0) {
            uint8_t l;
            if (!read_byte_raw(data, offset, l)) {
                last_error = DecompressError::Truncated;
                break;
            }
            length = l | ((ctrl & 0x03) << 8);
            mode = (ctrl << 3) & 0xE0;
        }
        // The low 5 bits (or the 10-bit extended value) hold length - 1 in
        // every mode, not just the extended runs (LC_LZ5 reference decoder).
        length += 1;

        try {
            switch (mode) {
                case 0x00: { // copy
                    if (offset + length > rom.size()) {
                        last_error = DecompressError::Truncated;
                        throw 1;
                    }
                    for (unsigned i = 0; i < length; ++i) {
                        out.push_back(data[offset + i]);
                    }
                    offset += length;
                    break;
                }
                case 0x20: { // byte repeat
                    uint8_t r;
                    if (!read_byte_raw(data, offset, r)) {
                        last_error = DecompressError::Truncated;
                        throw 1;
                    }
                    out.insert(out.end(), length, r);
                    break;
                }
                case 0x40: { // word repeat
                    uint8_t r0, r1;
                    if (!read_byte_raw(data, offset, r0) || !read_byte_raw(data, offset, r1)) {
                        last_error = DecompressError::Truncated;
                        throw 1;
                    }
                    out.reserve(out.size() + length);
                    for (unsigned i = 0; i < length; ++i) {
                        out.push_back((i & 1) ? r1 : r0);
                    }
                    break;
                }
                case 0x60: { // incrementing
                    uint8_t r;
                    if (!read_byte_raw(data, offset, r)) {
                        last_error = DecompressError::Truncated;
                        throw 1;
                    }
                    out.reserve(out.size() + length);
                    for (unsigned i = 0; i < length; ++i) {
                        out.push_back(r);
                        r = (r + 1) & 0xFF;
                    }
                    break;
                }
                default: { // back-reference modes
                    size_t ref;
                    if (mode & 0x40) { // relative
                        uint8_t rel;
                        if (!read_byte_raw(data, offset, rel)) {
                            last_error = DecompressError::Truncated;
                            throw 1;
                        }
                        if (out.size() < rel) {
                            last_error = DecompressError::BadRef;
                            throw 1;
                        }
                        ref = out.size() - rel;
                    } else { // absolute
                        uint16_t abs_ref;
                        if (!read_u16_raw(data, offset, abs_ref)) {
                            last_error = DecompressError::Truncated;
                            throw 1;
                        }
                        if (abs_ref >= out.size()) {
                            last_error = DecompressError::BadRef;
                            throw 1;
                        }
                        ref = abs_ref;
                    }
                    bool invert = mode & 0x20;
                    // LZ77-style backreference: source is the already-decoded
                    // output, allowing self-overlapping runs where the source
                    // region itself grows as bytes are appended.
                    out.reserve(out.size() + length);
                    for (unsigned i = 0; i < length; ++i) {
                        uint8_t v = out[ref + i];
                        if (invert) v ^= 0xFF;
                        out.push_back(v);
                    }
                    break;
                }
            }
        } catch (int) {
            break;
        }
    }

    out.clear();
    if (error_out) *error_out = last_error;
    return false;
}

} // namespace snes