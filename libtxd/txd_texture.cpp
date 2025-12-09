#include "txd_texture.h"
#include "txd_types.h"
#include <istream>
#include <ostream>
#include <cstring>
#include <algorithm>
#include <stdexcept>

namespace LibTXD {

Texture::Texture()
    : platform(Platform::D3D8)
    , filterFlags(0)
    , rasterFormat(RasterFormat::DEFAULT)
    , depth(32)
    , hasAlphaChannel(false)
    , compression(Compression::NONE)
    , paletteSize(0)
{
}

Texture::~Texture() = default;

Texture::Texture(Texture&& other) noexcept
    : platform(other.platform)
    , name(std::move(other.name))
    , maskName(std::move(other.maskName))
    , filterFlags(other.filterFlags)
    , rasterFormat(other.rasterFormat)
    , depth(other.depth)
    , hasAlphaChannel(other.hasAlphaChannel)
    , compression(other.compression)
    , mipmaps(std::move(other.mipmaps))
    , palette(std::move(other.palette))
    , paletteSize(other.paletteSize)
    , swizzleWidth(std::move(other.swizzleWidth))
    , swizzleHeight(std::move(other.swizzleHeight))
{
}

Texture& Texture::operator=(Texture&& other) noexcept {
    if (this != &other) {
        platform = other.platform;
        name = std::move(other.name);
        maskName = std::move(other.maskName);
        filterFlags = other.filterFlags;
        rasterFormat = other.rasterFormat;
        depth = other.depth;
        hasAlphaChannel = other.hasAlphaChannel;
        compression = other.compression;
        mipmaps = std::move(other.mipmaps);
        palette = std::move(other.palette);
        paletteSize = other.paletteSize;
        swizzleWidth = std::move(other.swizzleWidth);
        swizzleHeight = std::move(other.swizzleHeight);
    }
    return *this;
}

const MipmapLevel& Texture::getMipmap(size_t index) const {
    if (index >= mipmaps.size()) {
        throw std::out_of_range("Mipmap index out of range");
    }
    return mipmaps[index];
}

MipmapLevel& Texture::getMipmap(size_t index) {
    if (index >= mipmaps.size()) {
        throw std::out_of_range("Mipmap index out of range");
    }
    return mipmaps[index];
}

void Texture::addMipmap(MipmapLevel mipmap) {
    mipmaps.push_back(std::move(mipmap));
}

void Texture::setPalette(const std::vector<uint8_t>& pal, uint32_t size) {
    palette = pal;
    paletteSize = size;
}

void Texture::clear() {
    mipmaps.clear();
    palette.clear();
    paletteSize = 0;
    swizzleWidth.clear();
    swizzleHeight.clear();
}

bool Texture::readD3D(std::istream& stream) {
    ChunkHeader header;
    if (!header.read(stream)) {
        return false;
    }
    
    if (header.type != ChunkType::TEXTURENATIVE) {
        return false;
    }
    
    size_t sectionStart = stream.tellg();
    size_t sectionEnd = sectionStart + header.length;
    
    // Read struct section
    if (!readD3DStruct(stream, header)) {
        return false;
    }
    
    // Skip to end of section (there might be an extension section)
    stream.seekg(sectionEnd, std::ios::beg);
    
    return true;
}

bool Texture::readD3DStruct(std::istream& stream, ChunkHeader& parentHeader) {
    ChunkHeader structHeader;
    if (!structHeader.read(stream)) {
        return false;
    }
    
    if (structHeader.type != ChunkType::STRUCT) {
        return false;
    }
    
    size_t structStart = stream.tellg();
    size_t structEnd = structStart + structHeader.length;
    
    // Read platform
    uint32_t platformVal;
    stream.read(reinterpret_cast<char*>(&platformVal), 4);
    if (stream.gcount() != 4) {
        return false;
    }
    platform = static_cast<Platform>(fromLittleEndian32(platformVal));
    
    if (platform != Platform::D3D8 && platform != Platform::D3D9) {
        return false;
    }
    
    // Read filter flags
    stream.read(reinterpret_cast<char*>(&filterFlags), 4);
    filterFlags = fromLittleEndian32(filterFlags);
    
    // Read names (32 bytes each)
    char nameBuffer[32];
    stream.read(nameBuffer, 32);
    name = std::string(nameBuffer, strnlen(nameBuffer, 32));
    
    stream.read(nameBuffer, 32);
    maskName = std::string(nameBuffer, strnlen(nameBuffer, 32));
    
    // Read raster format
    uint32_t rasterFormatVal;
    stream.read(reinterpret_cast<char*>(&rasterFormatVal), 4);
    rasterFormat = static_cast<RasterFormat>(fromLittleEndian32(rasterFormatVal));
    
    // Read alpha/compression info
    hasAlphaChannel = false;
    compression = Compression::NONE;
    
    char fourcc[5] = {0};
    uint8_t compressionOrAlpha = 0;
    
    if (platform == Platform::D3D9) {
        stream.read(fourcc, 4);
    } else {
        uint32_t alphaVal;
        stream.read(reinterpret_cast<char*>(&alphaVal), 4);
        hasAlphaChannel = (fromLittleEndian32(alphaVal) == 1);
    }
    
    // Read dimensions
    uint16_t width, height;
    stream.read(reinterpret_cast<char*>(&width), 2);
    stream.read(reinterpret_cast<char*>(&height), 2);
    width = fromLittleEndian16(width);
    height = fromLittleEndian16(height);
    
    // Read depth
    stream.read(reinterpret_cast<char*>(&depth), 1);
    
    // Read mipmap count
    uint8_t mipmapCount;
    stream.read(reinterpret_cast<char*>(&mipmapCount), 1);
    
    // Skip raster type (always 4)
    stream.seekg(1, std::ios::cur);
    
    // Read compression/alpha
    stream.read(reinterpret_cast<char*>(&compressionOrAlpha), 1);
    
    if (platform == Platform::D3D9) {
        hasAlphaChannel = (compressionOrAlpha & 0x1) != 0;
        if (compressionOrAlpha & 0x8) {
            if (fourcc[0] == 'D' && fourcc[1] == 'X' && fourcc[2] == 'T') {
                if (fourcc[3] == '1') {
                    compression = Compression::DXT1;
                } else if (fourcc[3] == '3') {
                    compression = Compression::DXT3;
                }
            }
        } else {
            compression = Compression::NONE;
        }
    } else {
        if (compressionOrAlpha == 1) {
            compression = Compression::DXT1;
        } else if (compressionOrAlpha == 3) {
            compression = Compression::DXT3;
        }
    }
    
    // Read palette if present
    paletteSize = 0;
    if ((static_cast<uint32_t>(rasterFormat) & 0x2000) != 0) { // PAL8
        paletteSize = 256;
    } else if ((static_cast<uint32_t>(rasterFormat) & 0x4000) != 0) { // PAL4
        paletteSize = 16;
    }
    
    if (paletteSize > 0) {
        palette.resize(paletteSize * 4);
        stream.read(reinterpret_cast<char*>(palette.data()), paletteSize * 4);
    }
    
    // Read mipmaps
    mipmaps.clear();
    uint32_t currentWidth = width;
    uint32_t currentHeight = height;
    
    for (uint32_t i = 0; i < mipmapCount; i++) {
        if (i > 0) {
            currentWidth = std::max(1u, currentWidth / 2);
            currentHeight = std::max(1u, currentHeight / 2);
            
            // DXT compression works on 4x4 blocks
            if (compression != Compression::NONE) {
                if (currentWidth < 4 && currentWidth != 0) currentWidth = 4;
                if (currentHeight < 4 && currentHeight != 0) currentHeight = 4;
            }
        }
        
        // Read mipmap size
        uint32_t mipSize;
        stream.read(reinterpret_cast<char*>(&mipSize), 4);
        mipSize = fromLittleEndian32(mipSize);
        
        if (mipSize == 0) {
            currentWidth = currentHeight = 0;
        }
        
        MipmapLevel mipmap;
        mipmap.width = currentWidth;
        mipmap.height = currentHeight;
        mipmap.dataSize = mipSize;
        
        if (mipSize > 0) {
            mipmap.data.resize(mipSize);
            stream.read(reinterpret_cast<char*>(mipmap.data.data()), mipSize);
        }
        
        mipmaps.push_back(std::move(mipmap));
    }
    
    // Skip to end of struct
    stream.seekg(structEnd, std::ios::beg);
    
    return true;
}

bool Texture::readXbox(std::istream& stream) {
    // Xbox reading not fully implemented yet
    return false;
}

bool Texture::readPS2(std::istream& stream) {
    // PS2 reading not fully implemented yet
    return false;
}

uint32_t Texture::writeD3D(std::ostream& stream) const {
    size_t sectionStart = stream.tellp();
    
    // Write section header (will update later)
    ChunkHeader sectionHeader;
    sectionHeader.type = ChunkType::TEXTURENATIVE;
    sectionHeader.length = 0; // Will update
    sectionHeader.version = 0x34000; // Default SA version
    sectionHeader.write(stream);
    
    // Write struct
    uint32_t structSize = writeD3DStruct(stream);
    
    // Write extension section (empty)
    ChunkHeader extHeader;
    extHeader.type = ChunkType::EXTENSION;
    extHeader.length = 0;
    extHeader.version = sectionHeader.version;
    extHeader.write(stream);
    
    // Update section size
    size_t sectionEnd = stream.tellp();
    stream.seekp(sectionStart + 4, std::ios::beg);
    uint32_t sectionSize = toLittleEndian32(static_cast<uint32_t>(sectionEnd - sectionStart - 12));
    stream.write(reinterpret_cast<const char*>(&sectionSize), 4);
    stream.seekp(sectionEnd, std::ios::beg);
    
    return static_cast<uint32_t>(sectionEnd - sectionStart);
}

uint32_t Texture::writeD3DStruct(std::ostream& stream) const {
    size_t structStart = stream.tellp();
    
    // Write struct header (will update later)
    ChunkHeader structHeader;
    structHeader.type = ChunkType::STRUCT;
    structHeader.length = 0; // Will update
    structHeader.version = 0x34000;
    structHeader.write(stream);
    
    // Write platform
    uint32_t platformVal = toLittleEndian32(static_cast<uint32_t>(platform));
    stream.write(reinterpret_cast<const char*>(&platformVal), 4);
    
    // Write filter flags
    uint32_t filterFlagsVal = toLittleEndian32(filterFlags);
    stream.write(reinterpret_cast<const char*>(&filterFlagsVal), 4);
    
    // Write names (32 bytes each, null-padded)
    char nameBuffer[32] = {0};
    strncpy(nameBuffer, name.c_str(), 31);
    stream.write(nameBuffer, 32);
    
    strncpy(nameBuffer, maskName.c_str(), 31);
    stream.write(nameBuffer, 32);
    
    // Write raster format
    uint32_t rasterFormatVal = toLittleEndian32(static_cast<uint32_t>(rasterFormat));
    stream.write(reinterpret_cast<const char*>(&rasterFormatVal), 4);
    
    // Write alpha/compression
    if (platform == Platform::D3D8) {
        uint32_t alphaVal = toLittleEndian32(hasAlphaChannel ? 1 : 0);
        stream.write(reinterpret_cast<const char*>(&alphaVal), 4);
    } else { // D3D9
        if (compression != Compression::NONE) {
            char fourcc[4];
            if (compression == Compression::DXT1) {
                fourcc[0] = 'D'; fourcc[1] = 'X'; fourcc[2] = 'T'; fourcc[3] = '1';
            } else if (compression == Compression::DXT3) {
                fourcc[0] = 'D'; fourcc[1] = 'X'; fourcc[2] = 'T'; fourcc[3] = '3';
            }
            stream.write(fourcc, 4);
        } else {
            uint32_t value = hasAlphaChannel ? 0x15 : 0x16;
            uint32_t valueLE = toLittleEndian32(value);
            stream.write(reinterpret_cast<const char*>(&valueLE), 4);
        }
    }
    
    // Write dimensions
    uint16_t widthVal = toLittleEndian16(static_cast<uint16_t>(mipmaps.empty() ? 0 : mipmaps[0].width));
    uint16_t heightVal = toLittleEndian16(static_cast<uint16_t>(mipmaps.empty() ? 0 : mipmaps[0].height));
    stream.write(reinterpret_cast<const char*>(&widthVal), 2);
    stream.write(reinterpret_cast<const char*>(&heightVal), 2);
    
    // Write depth
    stream.write(reinterpret_cast<const char*>(&depth), 1);
    
    // Write mipmap count
    uint8_t mipmapCount = static_cast<uint8_t>(mipmaps.size());
    stream.write(reinterpret_cast<const char*>(&mipmapCount), 1);
    
    // Write raster type (always 4)
    uint8_t rasterType = 0x4;
    stream.write(reinterpret_cast<const char*>(&rasterType), 1);
    
    // Write compression/alpha
    uint8_t compressionOrAlpha;
    if (platform == Platform::D3D8) {
        compressionOrAlpha = static_cast<uint8_t>(compression);
    } else {
        compressionOrAlpha = (compression != Compression::NONE ? 8 : 0) | (hasAlphaChannel ? 1 : 0);
    }
    stream.write(reinterpret_cast<const char*>(&compressionOrAlpha), 1);
    
    // Write palette if present
    if (paletteSize > 0 && !palette.empty()) {
        stream.write(reinterpret_cast<const char*>(palette.data()), paletteSize * 4);
    }
    
    // Write mipmaps
    for (const auto& mipmap : mipmaps) {
        uint32_t mipSize = toLittleEndian32(mipmap.dataSize);
        stream.write(reinterpret_cast<const char*>(&mipSize), 4);
        
        if (mipmap.dataSize > 0 && !mipmap.data.empty()) {
            stream.write(reinterpret_cast<const char*>(mipmap.data.data()), mipmap.dataSize);
        }
    }
    
    // Update struct size
    size_t structEnd = stream.tellp();
    stream.seekp(structStart + 4, std::ios::beg);
    uint32_t structSize = toLittleEndian32(static_cast<uint32_t>(structEnd - structStart - 12));
    stream.write(reinterpret_cast<const char*>(&structSize), 4);
    stream.seekp(structEnd, std::ios::beg);
    
    return static_cast<uint32_t>(structEnd - structStart);
}

} // namespace LibTXD
