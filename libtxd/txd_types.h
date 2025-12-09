#ifndef TXD_TYPES_H
#define TXD_TYPES_H

#include <cstdint>
#include <string>
#include <vector>
#include <memory>
#include <istream>
#include <ostream>

namespace LibTXD {

// Platform identifiers
enum class Platform : uint32_t {
    OGL = 2,
    PS2 = 4,
    XBOX = 5,
    D3D8 = 8,
    D3D9 = 9,
    PS2_FOURCC = 0x00325350  // "PS2\0"
};

// Chunk types
enum class ChunkType : uint32_t {
    STRUCT = 0x01,
    STRING = 0x02,
    EXTENSION = 0x03,
    TEXTURENATIVE = 0x15,
    TEXDICTIONARY = 0x16,
    SKYMIPMAP = 0x110
};

// Raster formats
enum class RasterFormat : uint32_t {
    DEFAULT = 0x0000,
    A1R5G5B5 = 0x0100,
    R5G6B5 = 0x0200,
    R4G4B4A4 = 0x0300,
    LUM8 = 0x0400,
    B8G8R8A8 = 0x0500,
    B8G8R8 = 0x0600,
    R5G5B5 = 0x0A00,
    
    AUTOMIPMAP = 0x1000,
    PAL8 = 0x2000,
    PAL4 = 0x4000,
    MIPMAP = 0x8000,
    
    MASK = 0x0F00
};

// Compression types
enum class Compression : uint8_t {
    NONE = 0,
    DXT1 = 1,
    DXT3 = 3
};

// Game versions (for version detection)
enum class GameVersion : uint32_t {
    GTA3_1 = 0x00000302,
    GTA3_2 = 0x00000304,
    GTA3_3 = 0x00000310,
    GTA3_4 = 0x0800FFFF,
    VC_PS2 = 0x0C02FFFF,
    VC_PC = 0x1003FFFF,
    SA = 0x1803FFFF,
    UNKNOWN = 0
};

// Helper functions
inline uint32_t swapEndian32(uint32_t value) {
    return ((value & 0xFF000000) >> 24) |
           ((value & 0x00FF0000) >> 8) |
           ((value & 0x0000FF00) << 8) |
           ((value & 0x000000FF) << 24);
}

inline uint16_t swapEndian16(uint16_t value) {
    return ((value & 0xFF00) >> 8) | ((value & 0x00FF) << 8);
}

inline uint32_t toLittleEndian32(uint32_t value) {
#ifdef __BIG_ENDIAN__
    return swapEndian32(value);
#else
    return value;
#endif
}

inline uint16_t toLittleEndian16(uint16_t value) {
#ifdef __BIG_ENDIAN__
    return swapEndian16(value);
#else
    return value;
#endif
}

inline uint32_t fromLittleEndian32(uint32_t value) {
#ifdef __BIG_ENDIAN__
    return swapEndian32(value);
#else
    return value;
#endif
}

inline uint16_t fromLittleEndian16(uint16_t value) {
#ifdef __BIG_ENDIAN__
    return swapEndian16(value);
#else
    return value;
#endif
}

// Chunk header structure
struct ChunkHeader {
    ChunkType type;
    uint32_t length;
    uint32_t version;
    
    bool read(std::istream& stream);
    uint32_t write(std::ostream& stream) const;
};

} // namespace LibTXD

#endif // TXD_TYPES_H
