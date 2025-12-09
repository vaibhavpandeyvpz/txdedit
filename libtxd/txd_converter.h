#ifndef TXD_CONVERTER_H
#define TXD_CONVERTER_H

#include "txd_texture.h"
#include "txd_types.h"
#include <cstdint>
#include <vector>
#include <memory>

namespace LibTXD {

// Utility class for texture conversion operations
class TextureConverter {
public:
    // Decompress DXT compressed texture data to RGBA8
    // Returns nullptr on failure, or a buffer of width*height*4 bytes
    static std::unique_ptr<uint8_t[]> decompressDXT(
        const uint8_t* compressedData,
        uint32_t width,
        uint32_t height,
        Compression compression
    );
    
    // Compress RGBA8 data to DXT format
    // Returns nullptr on failure, or a buffer with compressed data
    static std::unique_ptr<uint8_t[]> compressToDXT(
        const uint8_t* rgbaData,
        uint32_t width,
        uint32_t height,
        Compression compression,
        float quality = 1.0f
    );
    
    // Get compressed data size for a given format and dimensions
    static size_t getCompressedDataSize(uint32_t width, uint32_t height, Compression compression);
    
    // Generate palette from RGBA8 image data using libimagequant
    // Returns true on success, false on failure
    // paletteSize: 16 for PAL4, 256 for PAL8
    static bool generatePalette(
        const uint8_t* rgbaData,
        uint32_t width,
        uint32_t height,
        uint32_t paletteSize,
        std::vector<uint8_t>& palette,  // Output: RGBA palette (paletteSize * 4 bytes)
        std::vector<uint8_t>& indexedData  // Output: Indexed image data (width * height bytes)
    );
    
    // Convert palette texture to RGBA8
    static void convertPaletteToRGBA(
        const uint8_t* indexedData,
        const uint8_t* palette,  // RGBA palette data
        uint32_t paletteSize,
        uint32_t width,
        uint32_t height,
        uint8_t* output  // Output RGBA8 buffer (width * height * 4 bytes)
    );
    
    // Convert texture mipmap to RGBA8 format
    // Handles uncompressed, DXT compressed, and palette textures
    static std::unique_ptr<uint8_t[]> convertToRGBA8(
        const Texture& texture,
        size_t mipmapIndex = 0
    );
    
    // Check if a texture format can be converted
    static bool canConvert(const Texture& texture);
    
private:
    // Helper: Convert uncompressed texture data to RGBA8
    static void convertUncompressed(
        const Texture& texture,
        const MipmapLevel& mipmap,
        uint8_t* output
    );
    
    // Helper: Convert DXT1 compressed data
    static void convertDXT1(
        const uint8_t* data,
        uint32_t width,
        uint32_t height,
        uint8_t* output
    );
    
    // Helper: Convert DXT3 compressed data
    static void convertDXT3(
        const uint8_t* data,
        uint32_t width,
        uint32_t height,
        uint8_t* output
    );
};

} // namespace LibTXD

#endif // TXD_CONVERTER_H
