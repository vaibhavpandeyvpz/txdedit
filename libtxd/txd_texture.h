#ifndef TXD_TEXTURE_H
#define TXD_TEXTURE_H

#include "txd_types.h"
#include <cstdint>
#include <string>
#include <vector>
#include <memory>
#include <iosfwd>

namespace LibTXD {

// Mipmap level data
struct MipmapLevel {
    uint32_t width;
    uint32_t height;
    uint32_t dataSize;
    std::vector<uint8_t> data;
    
    MipmapLevel() : width(0), height(0), dataSize(0) {}
};

// Texture class representing a native texture in a TXD file
class Texture {
public:
    Texture();
    ~Texture();
    
    // Non-copyable, movable
    Texture(const Texture&) = delete;
    Texture& operator=(const Texture&) = delete;
    Texture(Texture&&) noexcept;
    Texture& operator=(Texture&&) noexcept;
    
    // Getters
    Platform getPlatform() const { return platform; }
    const std::string& getName() const { return name; }
    const std::string& getMaskName() const { return maskName; }
    uint32_t getFilterFlags() const { return filterFlags; }
    RasterFormat getRasterFormat() const { return rasterFormat; }
    uint32_t getDepth() const { return depth; }
    uint32_t getMipmapCount() const { return static_cast<uint32_t>(mipmaps.size()); }
    bool hasAlpha() const { return hasAlphaChannel; }
    Compression getCompression() const { return compression; }
    
    const MipmapLevel& getMipmap(size_t index) const;
    MipmapLevel& getMipmap(size_t index);
    
    const std::vector<uint8_t>& getPalette() const { return palette; }
    uint32_t getPaletteSize() const { return paletteSize; }
    
    // Setters
    void setPlatform(Platform p) { platform = p; }
    void setName(const std::string& n) { name = n; }
    void setMaskName(const std::string& m) { maskName = m; }
    void setFilterFlags(uint32_t flags) { filterFlags = flags; }
    void setRasterFormat(RasterFormat format) { rasterFormat = format; }
    void setDepth(uint32_t d) { depth = d; }
    void setHasAlpha(bool alpha) { hasAlphaChannel = alpha; }
    void setCompression(Compression comp) { compression = comp; }
    
    void addMipmap(MipmapLevel mipmap);
    void setPalette(const std::vector<uint8_t>& pal, uint32_t size);
    
    // Reading
    bool readD3D(std::istream& stream);
    bool readXbox(std::istream& stream);
    bool readPS2(std::istream& stream);
    
    // Writing
    uint32_t writeD3D(std::ostream& stream) const;
    
    // Utility
    void clear();
    
private:
    Platform platform;
    std::string name;
    std::string maskName;
    uint32_t filterFlags;
    RasterFormat rasterFormat;
    uint32_t depth;
    bool hasAlphaChannel;
    Compression compression;
    
    std::vector<MipmapLevel> mipmaps;
    std::vector<uint8_t> palette;
    uint32_t paletteSize;
    
    // PS2 specific
    std::vector<uint32_t> swizzleWidth;
    std::vector<uint32_t> swizzleHeight;
    
    // Helper functions
    bool readD3DStruct(std::istream& stream, ChunkHeader& header);
    bool readXboxStruct(std::istream& stream, ChunkHeader& header);
    bool readPS2Struct(std::istream& stream, ChunkHeader& header);
    uint32_t writeD3DStruct(std::ostream& stream) const;
};

} // namespace LibTXD

#endif // TXD_TEXTURE_H
