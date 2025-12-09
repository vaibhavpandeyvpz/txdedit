#include "txd_converter.h"
#include <squish.h>
#include <libimagequant.h>
#include <cstring>
#include <algorithm>
#include <stdexcept>

namespace LibTXD {

std::unique_ptr<uint8_t[]> TextureConverter::decompressDXT(
    const uint8_t* compressedData,
    uint32_t width,
    uint32_t height,
    Compression compression) {
    
    if (!compressedData || width == 0 || height == 0) {
        return nullptr;
    }
    
    auto output = std::make_unique<uint8_t[]>(width * height * 4);
    
    int flags = 0;
    switch (compression) {
        case Compression::DXT1:
            flags = squish::kDxt1;
            break;
        case Compression::DXT3:
            flags = squish::kDxt3;
            break;
        default:
            return nullptr;
    }
    
    // Decompress using squish
    squish::DecompressImage(output.get(), static_cast<int>(width), static_cast<int>(height), compressedData, flags);
    
    return output;
}

std::unique_ptr<uint8_t[]> TextureConverter::compressToDXT(
    const uint8_t* rgbaData,
    uint32_t width,
    uint32_t height,
    Compression compression,
    float quality) {
    
    if (!rgbaData || width == 0 || height == 0) {
        return nullptr;
    }
    
    int flags = 0;
    switch (compression) {
        case Compression::DXT1:
            flags = squish::kDxt1;
            break;
        case Compression::DXT3:
            flags = squish::kDxt3;
            break;
        default:
            return nullptr;
    }
    
    // Use quality to select compression method
    if (quality >= 0.5f) {
        flags |= squish::kColourClusterFit;
    } else {
        flags |= squish::kColourRangeFit;
    }
    
    size_t compressedSize = getCompressedDataSize(width, height, compression);
    if (compressedSize == 0) {
        return nullptr;
    }
    
    auto compressedData = std::make_unique<uint8_t[]>(compressedSize);
    
    // Compress using squish
    squish::CompressImage(rgbaData, static_cast<int>(width), static_cast<int>(height), compressedData.get(), flags);
    
    return compressedData;
}

size_t TextureConverter::getCompressedDataSize(uint32_t width, uint32_t height, Compression compression) {
    int flags = 0;
    switch (compression) {
        case Compression::DXT1:
            flags = squish::kDxt1;
            break;
        case Compression::DXT3:
            flags = squish::kDxt3;
            break;
        default:
            return 0;
    }
    
    return squish::GetStorageRequirements(static_cast<int>(width), static_cast<int>(height), flags);
}

bool TextureConverter::generatePalette(
    const uint8_t* rgbaData,
    uint32_t width,
    uint32_t height,
    uint32_t paletteSize,
    std::vector<uint8_t>& palette,
    std::vector<uint8_t>& indexedData) {
    
    if (!rgbaData || width == 0 || height == 0 || (paletteSize != 16 && paletteSize != 256)) {
        return false;
    }
    
    // Create libimagequant attributes
    liq_attr* attr = liq_attr_create();
    if (!attr) {
        return false;
    }
    
    liq_set_max_colors(attr, static_cast<int>(paletteSize));
    liq_set_speed(attr, 5); // Balance between speed and quality
    
    // Create image from RGBA data
    liq_image* image = liq_image_create_rgba(attr, rgbaData, static_cast<int>(width), static_cast<int>(height), 0);
    if (!image) {
        liq_attr_destroy(attr);
        return false;
    }
    
    // Quantize the image
    liq_result* result = liq_quantize_image(attr, image);
    if (!result) {
        liq_image_destroy(image);
        liq_attr_destroy(attr);
        return false;
    }
    
    // Get palette
    const liq_palette* liq_pal = liq_get_palette(result);
    palette.clear();
    palette.reserve(paletteSize * 4);
    
    for (int i = 0; i < liq_pal->count; i++) {
        palette.push_back(liq_pal->entries[i].r);
        palette.push_back(liq_pal->entries[i].g);
        palette.push_back(liq_pal->entries[i].b);
        palette.push_back(liq_pal->entries[i].a);
    }
    
    // Pad palette to requested size if needed
    while (palette.size() < paletteSize * 4) {
        palette.push_back(0);
        palette.push_back(0);
        palette.push_back(0);
        palette.push_back(0);
    }
    
    // Remap image to indexed
    indexedData.resize(width * height);
    liq_write_remapped_image(result, image, indexedData.data(), width * height);
    
    // Cleanup
    liq_result_destroy(result);
    liq_image_destroy(image);
    liq_attr_destroy(attr);
    
    return true;
}

void TextureConverter::convertPaletteToRGBA(
    const uint8_t* indexedData,
    const uint8_t* palette,
    uint32_t paletteSize,
    uint32_t width,
    uint32_t height,
    uint8_t* output) {
    
    if (!indexedData || !palette || !output || width == 0 || height == 0) {
        return;
    }
    
    for (uint32_t y = 0; y < height; y++) {
        for (uint32_t x = 0; x < width; x++) {
            uint32_t index = indexedData[y * width + x];
            if (index >= paletteSize) {
                index = 0; // Safety check
            }
            
            uint32_t outIndex = (y * width + x) * 4;
            uint32_t palIndex = index * 4;
            
            output[outIndex + 0] = palette[palIndex + 0];  // R
            output[outIndex + 1] = palette[palIndex + 1];  // G
            output[outIndex + 2] = palette[palIndex + 2];  // B
            output[outIndex + 3] = palette[palIndex + 3];  // A
        }
    }
}

std::unique_ptr<uint8_t[]> TextureConverter::convertToRGBA8(
    const Texture& texture,
    size_t mipmapIndex) {
    
    if (mipmapIndex >= texture.getMipmapCount()) {
        return nullptr;
    }
    
    const auto& mipmap = texture.getMipmap(mipmapIndex);
    if (mipmap.width == 0 || mipmap.height == 0 || mipmap.data.empty()) {
        return nullptr;
    }
    
    auto output = std::make_unique<uint8_t[]>(mipmap.width * mipmap.height * 4);
    
    // Check for palette textures
    uint32_t rasterFormat = static_cast<uint32_t>(texture.getRasterFormat());
    bool isPalette = (rasterFormat & 0x2000) != 0 || (rasterFormat & 0x4000) != 0;
    
    if (isPalette) {
        // Handle palette texture
        uint32_t paletteSize = (rasterFormat & 0x2000) != 0 ? 256 : 16;
        const uint8_t* paletteData = mipmap.data.data();
        const uint8_t* indexedData = mipmap.data.data() + (paletteSize * 4);
        
        convertPaletteToRGBA(indexedData, paletteData, paletteSize, mipmap.width, mipmap.height, output.get());
    } else {
        // Convert based on compression
        switch (texture.getCompression()) {
            case Compression::DXT1:
                convertDXT1(mipmap.data.data(), mipmap.width, mipmap.height, output.get());
                break;
            case Compression::DXT3:
                convertDXT3(mipmap.data.data(), mipmap.width, mipmap.height, output.get());
                break;
            case Compression::NONE:
                convertUncompressed(texture, mipmap, output.get());
                break;
            default:
                // Unsupported compression
                std::memset(output.get(), 0, mipmap.width * mipmap.height * 4);
                break;
        }
    }
    
    return output;
}

bool TextureConverter::canConvert(const Texture& texture) {
    // Check for palette textures
    uint32_t rasterFormat = static_cast<uint32_t>(texture.getRasterFormat());
    bool isPalette = (rasterFormat & 0x2000) != 0 || (rasterFormat & 0x4000) != 0;
    if (isPalette) {
        return true;
    }
    
    // Support uncompressed, DXT1/DXT3
    bool supported = texture.getCompression() == Compression::NONE ||
                     texture.getCompression() == Compression::DXT1 ||
                     texture.getCompression() == Compression::DXT3;
    
    return supported;
}

void TextureConverter::convertUncompressed(
    const Texture& texture,
    const MipmapLevel& mipmap,
    uint8_t* output) {
    
    uint32_t format = static_cast<uint32_t>(texture.getRasterFormat());
    uint32_t formatMask = format & 0x0F00;
    uint8_t bpp = texture.getDepth() / 8;
    
    if (bpp == 0) {
        bpp = 4; // Default to 32-bit
    }
    
    for (uint32_t y = 0; y < mipmap.height; y++) {
        for (uint32_t x = 0; x < mipmap.width; x++) {
            uint32_t pixelIndex = y * mipmap.width + x;
            const uint8_t* pixelData = mipmap.data.data() + (pixelIndex * bpp);
            uint8_t* outPixel = output + (pixelIndex * 4);
            
            uint8_t r = 0, g = 0, b = 0, a = 255;
            
            switch (formatMask) {
                case 0x0500: // B8G8R8A8
                    b = pixelData[0];
                    g = pixelData[1];
                    r = pixelData[2];
                    a = pixelData[3];
                    break;
                    
                case 0x0600: // B8G8R8
                    b = pixelData[0];
                    g = pixelData[1];
                    r = pixelData[2];
                    a = 255;
                    break;
                    
                case 0x0200: { // R5G6B5
                    uint16_t pixel = pixelData[0] | (pixelData[1] << 8);
                    r = ((pixel >> 11) & 0x1F) << 3;
                    g = ((pixel >> 5) & 0x3F) << 2;
                    b = (pixel & 0x1F) << 3;
                    a = 255;
                    break;
                }
                
                case 0x0100: { // A1R5G5B5
                    uint16_t pixel = pixelData[0] | (pixelData[1] << 8);
                    a = ((pixel >> 15) & 0x1) ? 255 : 0;
                    r = ((pixel >> 10) & 0x1F) << 3;
                    g = ((pixel >> 5) & 0x1F) << 3;
                    b = (pixel & 0x1F) << 3;
                    break;
                }
                
                case 0x0300: { // R4G4B4A4
                    uint16_t pixel = pixelData[0] | (pixelData[1] << 8);
                    r = ((pixel >> 12) & 0xF) << 4;
                    g = ((pixel >> 8) & 0xF) << 4;
                    b = ((pixel >> 4) & 0xF) << 4;
                    a = (pixel & 0xF) << 4;
                    break;
                }
                
                case 0x0400: // LUM8
                    r = g = b = pixelData[0];
                    a = 255;
                    break;
                    
                default:
                    r = g = b = 0;
                    a = 255;
                    break;
            }
            
            outPixel[0] = r;
            outPixel[1] = g;
            outPixel[2] = b;
            outPixel[3] = a;
        }
    }
}

void TextureConverter::convertDXT1(
    const uint8_t* data,
    uint32_t width,
    uint32_t height,
    uint8_t* output) {
    
    auto decompressed = decompressDXT(data, width, height, Compression::DXT1);
    if (decompressed) {
        std::memcpy(output, decompressed.get(), width * height * 4);
    }
}

void TextureConverter::convertDXT3(
    const uint8_t* data,
    uint32_t width,
    uint32_t height,
    uint8_t* output) {
    
    auto decompressed = decompressDXT(data, width, height, Compression::DXT3);
    if (decompressed) {
        std::memcpy(output, decompressed.get(), width * height * 4);
    }
}

} // namespace LibTXD
