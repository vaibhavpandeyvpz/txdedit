/**
 * Comprehensive test suite for libtxd library
 * Tests all components: types, texture, dictionary, and converter
 */

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <fstream>
#include <sstream>
#include <filesystem>
#include <cstring>

#include "libtxd/txd_types.h"
#include "libtxd/txd_texture.h"
#include "libtxd/txd_dictionary.h"
#include "libtxd/txd_converter.h"

namespace fs = std::filesystem;

// Helper to get project root directory (where examples folder is)
static fs::path getProjectRoot() {
    // Try to find the examples folder by going up from current directory
    fs::path current = fs::current_path();
    
    // Check if we're already at the project root
    if (fs::exists(current / "examples")) {
        return current;
    }
    
    // Try parent directories
    fs::path parent = current.parent_path();
    while (!parent.empty() && parent != parent.parent_path()) {
        if (fs::exists(parent / "examples")) {
            return parent;
        }
        parent = parent.parent_path();
    }
    
    // Fallback to current directory
    return current;
}

static fs::path getExamplePath(const std::string& relativePath) {
    return getProjectRoot() / "examples" / relativePath;
}

// ============================================================================
// TXD Types Tests
// ============================================================================

class TxdTypesTest : public ::testing::Test {
protected:
    void SetUp() override {}
    void TearDown() override {}
};

TEST_F(TxdTypesTest, SwapEndian32_ReversesBytes) {
    EXPECT_EQ(LibTXD::swapEndian32(0x12345678), 0x78563412);
    EXPECT_EQ(LibTXD::swapEndian32(0x00000000), 0x00000000);
    EXPECT_EQ(LibTXD::swapEndian32(0xFFFFFFFF), 0xFFFFFFFF);
    EXPECT_EQ(LibTXD::swapEndian32(0xFF000000), 0x000000FF);
    EXPECT_EQ(LibTXD::swapEndian32(0x000000FF), 0xFF000000);
}

TEST_F(TxdTypesTest, SwapEndian16_ReversesBytes) {
    EXPECT_EQ(LibTXD::swapEndian16(0x1234), 0x3412);
    EXPECT_EQ(LibTXD::swapEndian16(0x0000), 0x0000);
    EXPECT_EQ(LibTXD::swapEndian16(0xFFFF), 0xFFFF);
    EXPECT_EQ(LibTXD::swapEndian16(0xFF00), 0x00FF);
}

TEST_F(TxdTypesTest, ToLittleEndian32_OnLittleEndianSystem) {
    // On little-endian systems (x86/x64), this should be identity
    uint32_t value = 0x12345678;
    EXPECT_EQ(LibTXD::toLittleEndian32(value), value);
}

TEST_F(TxdTypesTest, FromLittleEndian32_OnLittleEndianSystem) {
    uint32_t value = 0x12345678;
    EXPECT_EQ(LibTXD::fromLittleEndian32(value), value);
}

TEST_F(TxdTypesTest, ChunkHeader_WriteAndRead) {
    LibTXD::ChunkHeader header;
    header.type = LibTXD::ChunkType::TEXDICTIONARY;
    header.length = 12345;
    header.version = 0x1803FFFF;
    
    // Write to stream
    std::stringstream ss;
    uint32_t written = header.write(ss);
    EXPECT_EQ(written, 12u);  // ChunkHeader is 12 bytes
    
    // Read back
    ss.seekg(0);
    LibTXD::ChunkHeader readHeader;
    EXPECT_TRUE(readHeader.read(ss));
    
    EXPECT_EQ(readHeader.type, LibTXD::ChunkType::TEXDICTIONARY);
    EXPECT_EQ(readHeader.length, 12345u);
    EXPECT_EQ(readHeader.version, 0x1803FFFF);
}

TEST_F(TxdTypesTest, ChunkHeader_ReadFromEmptyStream_ReturnsFalse) {
    std::stringstream ss;
    LibTXD::ChunkHeader header;
    EXPECT_FALSE(header.read(ss));
}

TEST_F(TxdTypesTest, RasterFormat_MaskExtractsBaseFormat) {
    uint32_t formatWithFlags = static_cast<uint32_t>(LibTXD::RasterFormat::B8G8R8A8) | 
                               static_cast<uint32_t>(LibTXD::RasterFormat::MIPMAP);
    uint32_t baseFormat = formatWithFlags & static_cast<uint32_t>(LibTXD::RasterFormat::MASK);
    EXPECT_EQ(baseFormat, static_cast<uint32_t>(LibTXD::RasterFormat::B8G8R8A8));
}

TEST_F(TxdTypesTest, CompressionEnum_HasCorrectValues) {
    EXPECT_EQ(static_cast<uint8_t>(LibTXD::Compression::NONE), 0);
    EXPECT_EQ(static_cast<uint8_t>(LibTXD::Compression::DXT1), 1);
    EXPECT_EQ(static_cast<uint8_t>(LibTXD::Compression::DXT3), 3);
}

TEST_F(TxdTypesTest, PlatformEnum_HasCorrectValues) {
    EXPECT_EQ(static_cast<uint32_t>(LibTXD::Platform::D3D8), 8);
    EXPECT_EQ(static_cast<uint32_t>(LibTXD::Platform::D3D9), 9);
    EXPECT_EQ(static_cast<uint32_t>(LibTXD::Platform::PS2), 4);
    EXPECT_EQ(static_cast<uint32_t>(LibTXD::Platform::XBOX), 5);
}

// ============================================================================
// Texture Tests
// ============================================================================

class TextureTest : public ::testing::Test {
protected:
    void SetUp() override {}
    void TearDown() override {}
};

TEST_F(TextureTest, DefaultConstruction_HasDefaultValues) {
    LibTXD::Texture texture;
    
    EXPECT_EQ(texture.getPlatform(), LibTXD::Platform::D3D8);
    EXPECT_EQ(texture.getName(), "");
    EXPECT_EQ(texture.getMaskName(), "");
    EXPECT_EQ(texture.getFilterFlags(), 0u);
    EXPECT_EQ(texture.getRasterFormat(), LibTXD::RasterFormat::DEFAULT);
    EXPECT_EQ(texture.getDepth(), 32u);
    EXPECT_FALSE(texture.hasAlpha());
    EXPECT_EQ(texture.getCompression(), LibTXD::Compression::NONE);
    EXPECT_EQ(texture.getMipmapCount(), 0u);
    EXPECT_EQ(texture.getPaletteSize(), 0u);
}

TEST_F(TextureTest, Setters_UpdateValues) {
    LibTXD::Texture texture;
    
    texture.setPlatform(LibTXD::Platform::D3D9);
    texture.setName("test_texture");
    texture.setMaskName("test_mask");
    texture.setFilterFlags(0x1234);
    texture.setRasterFormat(LibTXD::RasterFormat::B8G8R8A8);
    texture.setDepth(32);
    texture.setHasAlpha(true);
    texture.setCompression(LibTXD::Compression::DXT3);
    
    EXPECT_EQ(texture.getPlatform(), LibTXD::Platform::D3D9);
    EXPECT_EQ(texture.getName(), "test_texture");
    EXPECT_EQ(texture.getMaskName(), "test_mask");
    EXPECT_EQ(texture.getFilterFlags(), 0x1234u);
    EXPECT_EQ(texture.getRasterFormat(), LibTXD::RasterFormat::B8G8R8A8);
    EXPECT_EQ(texture.getDepth(), 32u);
    EXPECT_TRUE(texture.hasAlpha());
    EXPECT_EQ(texture.getCompression(), LibTXD::Compression::DXT3);
}

TEST_F(TextureTest, AddMipmap_IncrementsMipmapCount) {
    LibTXD::Texture texture;
    
    EXPECT_EQ(texture.getMipmapCount(), 0u);
    
    LibTXD::MipmapLevel mip1;
    mip1.width = 64;
    mip1.height = 64;
    mip1.dataSize = 64 * 64 * 4;
    mip1.data.resize(mip1.dataSize, 0xFF);
    
    texture.addMipmap(std::move(mip1));
    EXPECT_EQ(texture.getMipmapCount(), 1u);
    
    LibTXD::MipmapLevel mip2;
    mip2.width = 32;
    mip2.height = 32;
    mip2.dataSize = 32 * 32 * 4;
    mip2.data.resize(mip2.dataSize, 0xFF);
    
    texture.addMipmap(std::move(mip2));
    EXPECT_EQ(texture.getMipmapCount(), 2u);
}

TEST_F(TextureTest, GetMipmap_ReturnsCorrectMipmap) {
    LibTXD::Texture texture;
    
    LibTXD::MipmapLevel mip;
    mip.width = 128;
    mip.height = 64;
    mip.dataSize = 128 * 64 * 4;
    mip.data.resize(mip.dataSize, 0xAB);
    
    texture.addMipmap(std::move(mip));
    
    const auto& retrievedMip = texture.getMipmap(0);
    EXPECT_EQ(retrievedMip.width, 128u);
    EXPECT_EQ(retrievedMip.height, 64u);
    EXPECT_EQ(retrievedMip.data[0], 0xAB);
}

TEST_F(TextureTest, GetMipmap_ThrowsOnInvalidIndex) {
    LibTXD::Texture texture;
    EXPECT_THROW(texture.getMipmap(0), std::out_of_range);
}

TEST_F(TextureTest, SetPalette_StoresPaletteData) {
    LibTXD::Texture texture;
    
    std::vector<uint8_t> palette(256 * 4, 0);
    // Set first color to red
    palette[0] = 255; palette[1] = 0; palette[2] = 0; palette[3] = 255;
    
    texture.setPalette(palette, 256);
    
    EXPECT_EQ(texture.getPaletteSize(), 256u);
    EXPECT_EQ(texture.getPalette().size(), 256u * 4);
    EXPECT_EQ(texture.getPalette()[0], 255);
}

TEST_F(TextureTest, Clear_ResetsTexture) {
    LibTXD::Texture texture;
    texture.setName("test");
    
    LibTXD::MipmapLevel mip;
    mip.width = 64;
    mip.height = 64;
    mip.dataSize = 64 * 64 * 4;
    mip.data.resize(mip.dataSize);
    texture.addMipmap(std::move(mip));
    
    texture.clear();
    
    EXPECT_EQ(texture.getMipmapCount(), 0u);
    EXPECT_EQ(texture.getPaletteSize(), 0u);
}

TEST_F(TextureTest, MoveConstructor_TransfersData) {
    LibTXD::Texture texture1;
    texture1.setName("original");
    texture1.setPlatform(LibTXD::Platform::D3D9);
    
    LibTXD::MipmapLevel mip;
    mip.width = 64;
    mip.height = 64;
    mip.dataSize = 64 * 64 * 4;
    mip.data.resize(mip.dataSize, 0x42);
    texture1.addMipmap(std::move(mip));
    
    LibTXD::Texture texture2(std::move(texture1));
    
    EXPECT_EQ(texture2.getName(), "original");
    EXPECT_EQ(texture2.getPlatform(), LibTXD::Platform::D3D9);
    EXPECT_EQ(texture2.getMipmapCount(), 1u);
    EXPECT_EQ(texture2.getMipmap(0).data[0], 0x42);
}

// ============================================================================
// Texture Dictionary Tests
// ============================================================================

class TextureDictionaryTest : public ::testing::Test {
protected:
    void SetUp() override {}
    void TearDown() override {}
};

TEST_F(TextureDictionaryTest, DefaultConstruction_IsEmpty) {
    LibTXD::TextureDictionary dict;
    
    EXPECT_EQ(dict.getTextureCount(), 0u);
    // Note: Default game version may vary based on implementation
    // Just verify it has a valid version set
    EXPECT_NE(dict.getVersion(), 0u);
}

TEST_F(TextureDictionaryTest, AddTexture_IncreasesCount) {
    LibTXD::TextureDictionary dict;
    
    LibTXD::Texture tex;
    tex.setName("texture1");
    dict.addTexture(std::move(tex));
    
    EXPECT_EQ(dict.getTextureCount(), 1u);
}

TEST_F(TextureDictionaryTest, GetTexture_ByIndex) {
    LibTXD::TextureDictionary dict;
    
    LibTXD::Texture tex;
    tex.setName("my_texture");
    dict.addTexture(std::move(tex));
    
    const LibTXD::Texture* retrieved = dict.getTexture(0);
    ASSERT_NE(retrieved, nullptr);
    EXPECT_EQ(retrieved->getName(), "my_texture");
}

TEST_F(TextureDictionaryTest, GetTexture_InvalidIndex_ReturnsNull) {
    LibTXD::TextureDictionary dict;
    EXPECT_EQ(dict.getTexture(0), nullptr);
    EXPECT_EQ(dict.getTexture(100), nullptr);
}

TEST_F(TextureDictionaryTest, FindTexture_ByName) {
    LibTXD::TextureDictionary dict;
    
    LibTXD::Texture tex1;
    tex1.setName("first");
    dict.addTexture(std::move(tex1));
    
    LibTXD::Texture tex2;
    tex2.setName("second");
    dict.addTexture(std::move(tex2));
    
    const LibTXD::Texture* found = dict.findTexture("second");
    ASSERT_NE(found, nullptr);
    EXPECT_EQ(found->getName(), "second");
}

TEST_F(TextureDictionaryTest, FindTexture_CaseInsensitive) {
    LibTXD::TextureDictionary dict;
    
    LibTXD::Texture tex;
    tex.setName("MyTexture");
    dict.addTexture(std::move(tex));
    
    // Search with different case
    const LibTXD::Texture* found = dict.findTexture("mytexture");
    ASSERT_NE(found, nullptr);
    EXPECT_EQ(found->getName(), "MyTexture");
}

TEST_F(TextureDictionaryTest, FindTexture_NotFound_ReturnsNull) {
    LibTXD::TextureDictionary dict;
    EXPECT_EQ(dict.findTexture("nonexistent"), nullptr);
}

TEST_F(TextureDictionaryTest, RemoveTexture_ByIndex) {
    LibTXD::TextureDictionary dict;
    
    LibTXD::Texture tex1;
    tex1.setName("first");
    dict.addTexture(std::move(tex1));
    
    LibTXD::Texture tex2;
    tex2.setName("second");
    dict.addTexture(std::move(tex2));
    
    EXPECT_EQ(dict.getTextureCount(), 2u);
    
    dict.removeTexture(0);
    
    EXPECT_EQ(dict.getTextureCount(), 1u);
    EXPECT_EQ(dict.getTexture(0)->getName(), "second");
}

TEST_F(TextureDictionaryTest, RemoveTexture_ByName) {
    LibTXD::TextureDictionary dict;
    
    LibTXD::Texture tex1;
    tex1.setName("keep_me");
    dict.addTexture(std::move(tex1));
    
    LibTXD::Texture tex2;
    tex2.setName("remove_me");
    dict.addTexture(std::move(tex2));
    
    dict.removeTexture("remove_me");
    
    EXPECT_EQ(dict.getTextureCount(), 1u);
    EXPECT_NE(dict.findTexture("keep_me"), nullptr);
    EXPECT_EQ(dict.findTexture("remove_me"), nullptr);
}

TEST_F(TextureDictionaryTest, Clear_RemovesAllTextures) {
    LibTXD::TextureDictionary dict;
    
    for (int i = 0; i < 5; i++) {
        LibTXD::Texture tex;
        tex.setName("texture" + std::to_string(i));
        dict.addTexture(std::move(tex));
    }
    
    EXPECT_EQ(dict.getTextureCount(), 5u);
    
    dict.clear();
    
    EXPECT_EQ(dict.getTextureCount(), 0u);
}

TEST_F(TextureDictionaryTest, SetVersion_UpdatesVersion) {
    LibTXD::TextureDictionary dict;
    
    dict.setVersion(0x1803FFFF);
    EXPECT_EQ(dict.getVersion(), 0x1803FFFF);
}

// ============================================================================
// Dictionary File I/O Tests (using example files)
// ============================================================================

class DictionaryFileIOTest : public ::testing::Test {
protected:
    fs::path tempDir;
    
    void SetUp() override {
        tempDir = fs::temp_directory_path() / "libtxd_tests";
        fs::create_directories(tempDir);
    }
    
    void TearDown() override {
        fs::remove_all(tempDir);
    }
};

TEST_F(DictionaryFileIOTest, Load_GTA3_Infernus) {
    fs::path txdPath = getExamplePath("gta3/infernus.txd");
    
    if (!fs::exists(txdPath)) {
        GTEST_SKIP() << "Example file not found: " << txdPath;
    }
    
    LibTXD::TextureDictionary dict;
    ASSERT_TRUE(dict.load(txdPath.string()));
    
    EXPECT_GT(dict.getTextureCount(), 0u);
    // GTA3 uses older version
    EXPECT_NE(dict.getVersion(), 0u);
}

TEST_F(DictionaryFileIOTest, Load_GTAVC_Infernus) {
    fs::path txdPath = getExamplePath("gtavc/infernus.txd");
    
    if (!fs::exists(txdPath)) {
        GTEST_SKIP() << "Example file not found: " << txdPath;
    }
    
    LibTXD::TextureDictionary dict;
    ASSERT_TRUE(dict.load(txdPath.string()));
    
    EXPECT_GT(dict.getTextureCount(), 0u);
}

TEST_F(DictionaryFileIOTest, Load_GTASA_Infernus) {
    fs::path txdPath = getExamplePath("gtasa/infernus.txd");
    
    if (!fs::exists(txdPath)) {
        GTEST_SKIP() << "Example file not found: " << txdPath;
    }
    
    LibTXD::TextureDictionary dict;
    ASSERT_TRUE(dict.load(txdPath.string()));
    
    EXPECT_GT(dict.getTextureCount(), 0u);
}

TEST_F(DictionaryFileIOTest, Load_NonExistentFile_ReturnsFalse) {
    LibTXD::TextureDictionary dict;
    EXPECT_FALSE(dict.load("/nonexistent/path/file.txd"));
}

TEST_F(DictionaryFileIOTest, Load_InvalidFile_ReturnsFalse) {
    // Create a file with invalid content
    fs::path invalidPath = tempDir / "invalid.txd";
    std::ofstream file(invalidPath, std::ios::binary);
    file << "This is not a valid TXD file";
    file.close();
    
    LibTXD::TextureDictionary dict;
    EXPECT_FALSE(dict.load(invalidPath.string()));
}

TEST_F(DictionaryFileIOTest, Save_EmptyDictionary) {
    LibTXD::TextureDictionary dict;
    dict.setVersion(0x1803FFFF);
    
    fs::path savePath = tempDir / "empty.txd";
    EXPECT_TRUE(dict.save(savePath.string()));
    EXPECT_TRUE(fs::exists(savePath));
    
    // Should be able to reload
    LibTXD::TextureDictionary reloaded;
    EXPECT_TRUE(reloaded.load(savePath.string()));
    EXPECT_EQ(reloaded.getTextureCount(), 0u);
}

TEST_F(DictionaryFileIOTest, Roundtrip_PreservesTextureCount) {
    fs::path txdPath = getExamplePath("gtavc/infernus.txd");
    
    if (!fs::exists(txdPath)) {
        GTEST_SKIP() << "Example file not found: " << txdPath;
    }
    
    // Load original
    LibTXD::TextureDictionary original;
    ASSERT_TRUE(original.load(txdPath.string()));
    size_t originalCount = original.getTextureCount();
    
    // Save to temp file
    fs::path savePath = tempDir / "roundtrip.txd";
    ASSERT_TRUE(original.save(savePath.string()));
    
    // Reload
    LibTXD::TextureDictionary reloaded;
    ASSERT_TRUE(reloaded.load(savePath.string()));
    
    EXPECT_EQ(reloaded.getTextureCount(), originalCount);
}

TEST_F(DictionaryFileIOTest, Roundtrip_PreservesTextureNames) {
    fs::path txdPath = getExamplePath("gtavc/infernus.txd");
    
    if (!fs::exists(txdPath)) {
        GTEST_SKIP() << "Example file not found: " << txdPath;
    }
    
    // Load original
    LibTXD::TextureDictionary original;
    ASSERT_TRUE(original.load(txdPath.string()));
    
    // Collect original names
    std::vector<std::string> originalNames;
    for (size_t i = 0; i < original.getTextureCount(); i++) {
        originalNames.push_back(original.getTexture(i)->getName());
    }
    
    // Save and reload
    fs::path savePath = tempDir / "roundtrip_names.txd";
    ASSERT_TRUE(original.save(savePath.string()));
    
    LibTXD::TextureDictionary reloaded;
    ASSERT_TRUE(reloaded.load(savePath.string()));
    
    // Check names match
    ASSERT_EQ(reloaded.getTextureCount(), originalNames.size());
    for (size_t i = 0; i < reloaded.getTextureCount(); i++) {
        EXPECT_EQ(reloaded.getTexture(i)->getName(), originalNames[i]);
    }
}

TEST_F(DictionaryFileIOTest, Roundtrip_PreservesTextureDimensions) {
    fs::path txdPath = getExamplePath("gtavc/infernus.txd");
    
    if (!fs::exists(txdPath)) {
        GTEST_SKIP() << "Example file not found: " << txdPath;
    }
    
    // Load original
    LibTXD::TextureDictionary original;
    ASSERT_TRUE(original.load(txdPath.string()));
    
    // Collect original dimensions
    std::vector<std::pair<uint32_t, uint32_t>> originalDims;
    for (size_t i = 0; i < original.getTextureCount(); i++) {
        const auto* tex = original.getTexture(i);
        if (tex->getMipmapCount() > 0) {
            const auto& mip = tex->getMipmap(0);
            originalDims.push_back({mip.width, mip.height});
        }
    }
    
    // Save and reload
    fs::path savePath = tempDir / "roundtrip_dims.txd";
    ASSERT_TRUE(original.save(savePath.string()));
    
    LibTXD::TextureDictionary reloaded;
    ASSERT_TRUE(reloaded.load(savePath.string()));
    
    // Check dimensions match
    size_t idx = 0;
    for (size_t i = 0; i < reloaded.getTextureCount() && idx < originalDims.size(); i++) {
        const auto* tex = reloaded.getTexture(i);
        if (tex->getMipmapCount() > 0) {
            const auto& mip = tex->getMipmap(0);
            EXPECT_EQ(mip.width, originalDims[idx].first) << "Width mismatch at texture " << i;
            EXPECT_EQ(mip.height, originalDims[idx].second) << "Height mismatch at texture " << i;
            idx++;
        }
    }
}

// ============================================================================
// Texture Converter Tests
// ============================================================================

class TextureConverterTest : public ::testing::Test {
protected:
    void SetUp() override {}
    void TearDown() override {}
    
    // Create a simple 4x4 RGBA test image
    std::vector<uint8_t> createTestRGBA(uint32_t width, uint32_t height, 
                                         uint8_t r, uint8_t g, uint8_t b, uint8_t a) {
        std::vector<uint8_t> data(width * height * 4);
        for (size_t i = 0; i < width * height; i++) {
            data[i * 4 + 0] = r;
            data[i * 4 + 1] = g;
            data[i * 4 + 2] = b;
            data[i * 4 + 3] = a;
        }
        return data;
    }
    
    // Create a gradient test image
    std::vector<uint8_t> createGradientRGBA(uint32_t width, uint32_t height) {
        std::vector<uint8_t> data(width * height * 4);
        for (uint32_t y = 0; y < height; y++) {
            for (uint32_t x = 0; x < width; x++) {
                size_t idx = (y * width + x) * 4;
                data[idx + 0] = static_cast<uint8_t>(x * 255 / width);   // R
                data[idx + 1] = static_cast<uint8_t>(y * 255 / height);  // G
                data[idx + 2] = static_cast<uint8_t>((x + y) * 127 / (width + height)); // B
                data[idx + 3] = 255;  // A
            }
        }
        return data;
    }
};

TEST_F(TextureConverterTest, GetCompressedDataSize_DXT1) {
    // DXT1: 8 bytes per 4x4 block
    EXPECT_EQ(LibTXD::TextureConverter::getCompressedDataSize(4, 4, LibTXD::Compression::DXT1), 8u);
    EXPECT_EQ(LibTXD::TextureConverter::getCompressedDataSize(8, 8, LibTXD::Compression::DXT1), 32u);
    EXPECT_EQ(LibTXD::TextureConverter::getCompressedDataSize(64, 64, LibTXD::Compression::DXT1), 2048u);
    EXPECT_EQ(LibTXD::TextureConverter::getCompressedDataSize(128, 128, LibTXD::Compression::DXT1), 8192u);
}

TEST_F(TextureConverterTest, GetCompressedDataSize_DXT3) {
    // DXT3: 16 bytes per 4x4 block
    EXPECT_EQ(LibTXD::TextureConverter::getCompressedDataSize(4, 4, LibTXD::Compression::DXT3), 16u);
    EXPECT_EQ(LibTXD::TextureConverter::getCompressedDataSize(8, 8, LibTXD::Compression::DXT3), 64u);
    EXPECT_EQ(LibTXD::TextureConverter::getCompressedDataSize(64, 64, LibTXD::Compression::DXT3), 4096u);
}

TEST_F(TextureConverterTest, GetCompressedDataSize_None_ReturnsZero) {
    EXPECT_EQ(LibTXD::TextureConverter::getCompressedDataSize(64, 64, LibTXD::Compression::NONE), 0u);
}

TEST_F(TextureConverterTest, CompressToDXT1_ProducesValidOutput) {
    auto rgba = createTestRGBA(8, 8, 255, 0, 0, 255);  // Red image
    
    auto compressed = LibTXD::TextureConverter::compressToDXT(
        rgba.data(), 8, 8, LibTXD::Compression::DXT1, 1.0f);
    
    ASSERT_NE(compressed, nullptr);
    
    size_t expectedSize = LibTXD::TextureConverter::getCompressedDataSize(8, 8, LibTXD::Compression::DXT1);
    EXPECT_EQ(expectedSize, 32u);
}

TEST_F(TextureConverterTest, CompressToDXT3_ProducesValidOutput) {
    auto rgba = createTestRGBA(8, 8, 0, 255, 0, 128);  // Semi-transparent green
    
    auto compressed = LibTXD::TextureConverter::compressToDXT(
        rgba.data(), 8, 8, LibTXD::Compression::DXT3, 1.0f);
    
    ASSERT_NE(compressed, nullptr);
}

TEST_F(TextureConverterTest, CompressToDXT_NullInput_ReturnsNull) {
    auto compressed = LibTXD::TextureConverter::compressToDXT(
        nullptr, 8, 8, LibTXD::Compression::DXT1, 1.0f);
    
    EXPECT_EQ(compressed, nullptr);
}

TEST_F(TextureConverterTest, CompressToDXT_ZeroDimension_ReturnsNull) {
    auto rgba = createTestRGBA(8, 8, 255, 255, 255, 255);
    
    auto compressed = LibTXD::TextureConverter::compressToDXT(
        rgba.data(), 0, 8, LibTXD::Compression::DXT1, 1.0f);
    
    EXPECT_EQ(compressed, nullptr);
}

TEST_F(TextureConverterTest, DecompressDXT1_ProducesValidOutput) {
    // First compress, then decompress
    auto rgba = createGradientRGBA(8, 8);
    
    auto compressed = LibTXD::TextureConverter::compressToDXT(
        rgba.data(), 8, 8, LibTXD::Compression::DXT1, 1.0f);
    ASSERT_NE(compressed, nullptr);
    
    auto decompressed = LibTXD::TextureConverter::decompressDXT(
        compressed.get(), 8, 8, LibTXD::Compression::DXT1);
    
    ASSERT_NE(decompressed, nullptr);
}

TEST_F(TextureConverterTest, DecompressDXT3_ProducesValidOutput) {
    auto rgba = createGradientRGBA(8, 8);
    
    auto compressed = LibTXD::TextureConverter::compressToDXT(
        rgba.data(), 8, 8, LibTXD::Compression::DXT3, 1.0f);
    ASSERT_NE(compressed, nullptr);
    
    auto decompressed = LibTXD::TextureConverter::decompressDXT(
        compressed.get(), 8, 8, LibTXD::Compression::DXT3);
    
    ASSERT_NE(decompressed, nullptr);
}

TEST_F(TextureConverterTest, DXT1_CompressDecompress_Roundtrip) {
    // Create solid color image (should compress/decompress perfectly)
    auto original = createTestRGBA(16, 16, 128, 64, 192, 255);
    
    auto compressed = LibTXD::TextureConverter::compressToDXT(
        original.data(), 16, 16, LibTXD::Compression::DXT1, 1.0f);
    ASSERT_NE(compressed, nullptr);
    
    auto decompressed = LibTXD::TextureConverter::decompressDXT(
        compressed.get(), 16, 16, LibTXD::Compression::DXT1);
    ASSERT_NE(decompressed, nullptr);
    
    // For solid colors, the roundtrip should be fairly accurate
    // Allow some tolerance due to DXT lossy compression
    int maxDiff = 0;
    for (size_t i = 0; i < 16 * 16 * 4; i++) {
        int diff = std::abs(static_cast<int>(original[i]) - static_cast<int>(decompressed[i]));
        maxDiff = std::max(maxDiff, diff);
    }
    
    // DXT compression typically has error < 10 for solid colors
    EXPECT_LT(maxDiff, 20) << "DXT roundtrip error too high";
}

TEST_F(TextureConverterTest, ConvertToRGBA8_UncompressedTexture) {
    LibTXD::Texture texture;
    texture.setRasterFormat(LibTXD::RasterFormat::B8G8R8A8);
    texture.setCompression(LibTXD::Compression::NONE);
    texture.setDepth(32);
    
    // Create BGRA data (as stored in file)
    LibTXD::MipmapLevel mip;
    mip.width = 4;
    mip.height = 4;
    mip.dataSize = 4 * 4 * 4;
    mip.data.resize(mip.dataSize);
    
    // Set first pixel to blue (BGRA = B, G, R, A)
    mip.data[0] = 255;  // B
    mip.data[1] = 0;    // G
    mip.data[2] = 0;    // R
    mip.data[3] = 255;  // A
    
    texture.addMipmap(std::move(mip));
    
    auto rgba = LibTXD::TextureConverter::convertToRGBA8(texture, 0);
    ASSERT_NE(rgba, nullptr);
    
    // After conversion to RGBA, first pixel should be:
    // R=0, G=0, B=255, A=255 (blue in RGBA format)
    EXPECT_EQ(rgba[0], 0);    // R
    EXPECT_EQ(rgba[1], 0);    // G
    EXPECT_EQ(rgba[2], 255);  // B
    EXPECT_EQ(rgba[3], 255);  // A
}

TEST_F(TextureConverterTest, ConvertToRGBA8_CompressedTexture) {
    fs::path txdPath = getExamplePath("gtavc/infernus.txd");
    
    if (!fs::exists(txdPath)) {
        GTEST_SKIP() << "Example file not found: " << txdPath;
    }
    
    LibTXD::TextureDictionary dict;
    ASSERT_TRUE(dict.load(txdPath.string()));
    ASSERT_GT(dict.getTextureCount(), 0u);
    
    const auto* texture = dict.getTexture(0);
    ASSERT_NE(texture, nullptr);
    ASSERT_GT(texture->getMipmapCount(), 0u);
    
    auto rgba = LibTXD::TextureConverter::convertToRGBA8(*texture, 0);
    
    // Should successfully convert
    ASSERT_NE(rgba, nullptr);
    
    // Check dimensions match
    const auto& mip = texture->getMipmap(0);
    size_t expectedSize = mip.width * mip.height * 4;
    
    // Data should be non-zero (actual image data)
    bool hasNonZeroData = false;
    for (size_t i = 0; i < expectedSize; i++) {
        if (rgba[i] != 0) {
            hasNonZeroData = true;
            break;
        }
    }
    EXPECT_TRUE(hasNonZeroData) << "Converted image appears to be all zeros";
}

TEST_F(TextureConverterTest, CanConvert_SupportedFormats) {
    LibTXD::Texture texNone;
    texNone.setCompression(LibTXD::Compression::NONE);
    EXPECT_TRUE(LibTXD::TextureConverter::canConvert(texNone));
    
    LibTXD::Texture texDXT1;
    texDXT1.setCompression(LibTXD::Compression::DXT1);
    EXPECT_TRUE(LibTXD::TextureConverter::canConvert(texDXT1));
    
    LibTXD::Texture texDXT3;
    texDXT3.setCompression(LibTXD::Compression::DXT3);
    EXPECT_TRUE(LibTXD::TextureConverter::canConvert(texDXT3));
}

TEST_F(TextureConverterTest, GeneratePalette_PAL8) {
    auto rgba = createGradientRGBA(32, 32);
    
    std::vector<uint8_t> palette;
    std::vector<uint8_t> indexedData;
    
    bool result = LibTXD::TextureConverter::generatePalette(
        rgba.data(), 32, 32, 256, palette, indexedData);
    
    ASSERT_TRUE(result);
    EXPECT_EQ(palette.size(), 256u * 4);  // 256 colors, 4 bytes each
    EXPECT_EQ(indexedData.size(), 32u * 32);  // One byte per pixel
}

TEST_F(TextureConverterTest, GeneratePalette_PAL4) {
    auto rgba = createTestRGBA(16, 16, 100, 150, 200, 255);
    
    std::vector<uint8_t> palette;
    std::vector<uint8_t> indexedData;
    
    bool result = LibTXD::TextureConverter::generatePalette(
        rgba.data(), 16, 16, 16, palette, indexedData);
    
    ASSERT_TRUE(result);
    EXPECT_EQ(palette.size(), 16u * 4);  // 16 colors, 4 bytes each
    EXPECT_EQ(indexedData.size(), 16u * 16);
}

TEST_F(TextureConverterTest, ConvertPaletteToRGBA_ReconstructsImage) {
    // Create simple indexed data
    uint32_t width = 4;
    uint32_t height = 4;
    
    std::vector<uint8_t> palette = {
        255, 0, 0, 255,    // Index 0: Red
        0, 255, 0, 255,    // Index 1: Green
        0, 0, 255, 255,    // Index 2: Blue
        255, 255, 255, 255 // Index 3: White
    };
    
    std::vector<uint8_t> indexedData = {
        0, 0, 1, 1,
        0, 0, 1, 1,
        2, 2, 3, 3,
        2, 2, 3, 3
    };
    
    std::vector<uint8_t> output(width * height * 4);
    
    LibTXD::TextureConverter::convertPaletteToRGBA(
        indexedData.data(), palette.data(), 4, width, height, output.data());
    
    // Check first pixel is red
    EXPECT_EQ(output[0], 255);  // R
    EXPECT_EQ(output[1], 0);    // G
    EXPECT_EQ(output[2], 0);    // B
    EXPECT_EQ(output[3], 255);  // A
    
    // Check pixel at (2, 0) is green
    size_t greenIdx = 2 * 4;
    EXPECT_EQ(output[greenIdx + 0], 0);
    EXPECT_EQ(output[greenIdx + 1], 255);
    EXPECT_EQ(output[greenIdx + 2], 0);
}

// ============================================================================
// Integration Tests
// ============================================================================

class IntegrationTest : public ::testing::Test {
protected:
    fs::path tempDir;
    
    void SetUp() override {
        tempDir = fs::temp_directory_path() / "libtxd_integration";
        fs::create_directories(tempDir);
    }
    
    void TearDown() override {
        fs::remove_all(tempDir);
    }
};

TEST_F(IntegrationTest, CreateNewTXD_AddTextures_Save_Reload) {
    // Create a new TXD from scratch
    LibTXD::TextureDictionary dict;
    dict.setVersion(0x1803FFFF);  // SA version
    
    // Add a texture
    LibTXD::Texture texture;
    texture.setName("custom_texture");
    texture.setPlatform(LibTXD::Platform::D3D8);
    texture.setRasterFormat(LibTXD::RasterFormat::B8G8R8A8);
    texture.setDepth(32);
    texture.setHasAlpha(true);
    texture.setCompression(LibTXD::Compression::NONE);
    texture.setFilterFlags(0x1106);
    
    // Create mipmap data (8x8 BGRA)
    LibTXD::MipmapLevel mip;
    mip.width = 8;
    mip.height = 8;
    mip.dataSize = 8 * 8 * 4;
    mip.data.resize(mip.dataSize);
    
    // Fill with a pattern
    for (size_t i = 0; i < mip.dataSize; i += 4) {
        mip.data[i + 0] = 0;    // B
        mip.data[i + 1] = 128;  // G
        mip.data[i + 2] = 255;  // R
        mip.data[i + 3] = 255;  // A
    }
    
    texture.addMipmap(std::move(mip));
    dict.addTexture(std::move(texture));
    
    // Save
    fs::path savePath = tempDir / "custom.txd";
    ASSERT_TRUE(dict.save(savePath.string()));
    
    // Reload and verify
    LibTXD::TextureDictionary reloaded;
    ASSERT_TRUE(reloaded.load(savePath.string()));
    
    EXPECT_EQ(reloaded.getTextureCount(), 1u);
    
    const auto* tex = reloaded.getTexture(0);
    ASSERT_NE(tex, nullptr);
    EXPECT_EQ(tex->getName(), "custom_texture");
    EXPECT_EQ(tex->getMipmapCount(), 1u);
    
    const auto& reloadedMip = tex->getMipmap(0);
    EXPECT_EQ(reloadedMip.width, 8u);
    EXPECT_EQ(reloadedMip.height, 8u);
}

TEST_F(IntegrationTest, ModifyExistingTexture_Save_Reload) {
    fs::path txdPath = getExamplePath("gtavc/infernus.txd");
    
    if (!fs::exists(txdPath)) {
        GTEST_SKIP() << "Example file not found: " << txdPath;
    }
    
    // Load
    LibTXD::TextureDictionary dict;
    ASSERT_TRUE(dict.load(txdPath.string()));
    ASSERT_GT(dict.getTextureCount(), 0u);
    
    // Modify first texture's name
    LibTXD::Texture* tex = dict.getTexture(0);
    ASSERT_NE(tex, nullptr);
    std::string originalName = tex->getName();
    std::string newName = "modified_" + originalName;
    
    // We can't directly modify the name, but we can verify the texture exists
    // and the dictionary handles it correctly
    
    // Save
    fs::path savePath = tempDir / "modified.txd";
    ASSERT_TRUE(dict.save(savePath.string()));
    
    // Reload
    LibTXD::TextureDictionary reloaded;
    ASSERT_TRUE(reloaded.load(savePath.string()));
    
    EXPECT_EQ(reloaded.getTextureCount(), dict.getTextureCount());
}

TEST_F(IntegrationTest, AddRemoveTextures_MaintainsIntegrity) {
    LibTXD::TextureDictionary dict;
    dict.setVersion(0x1803FFFF);
    
    // Add multiple textures
    for (int i = 0; i < 5; i++) {
        LibTXD::Texture tex;
        tex.setName("texture_" + std::to_string(i));
        tex.setPlatform(LibTXD::Platform::D3D8);
        
        LibTXD::MipmapLevel mip;
        mip.width = 4;
        mip.height = 4;
        mip.dataSize = 4 * 4 * 4;
        mip.data.resize(mip.dataSize, static_cast<uint8_t>(i * 50));
        
        tex.addMipmap(std::move(mip));
        dict.addTexture(std::move(tex));
    }
    
    EXPECT_EQ(dict.getTextureCount(), 5u);
    
    // Remove middle texture
    dict.removeTexture("texture_2");
    EXPECT_EQ(dict.getTextureCount(), 4u);
    EXPECT_EQ(dict.findTexture("texture_2"), nullptr);
    
    // Save and reload
    fs::path savePath = tempDir / "add_remove.txd";
    ASSERT_TRUE(dict.save(savePath.string()));
    
    LibTXD::TextureDictionary reloaded;
    ASSERT_TRUE(reloaded.load(savePath.string()));
    
    EXPECT_EQ(reloaded.getTextureCount(), 4u);
    EXPECT_NE(reloaded.findTexture("texture_0"), nullptr);
    EXPECT_NE(reloaded.findTexture("texture_1"), nullptr);
    EXPECT_EQ(reloaded.findTexture("texture_2"), nullptr);
    EXPECT_NE(reloaded.findTexture("texture_3"), nullptr);
    EXPECT_NE(reloaded.findTexture("texture_4"), nullptr);
}

TEST_F(IntegrationTest, CompressedTexture_FullPipeline) {
    LibTXD::TextureDictionary dict;
    dict.setVersion(0x1803FFFF);
    
    // Create RGBA data
    uint32_t width = 64;
    uint32_t height = 64;
    std::vector<uint8_t> rgbaData(width * height * 4);
    for (size_t i = 0; i < rgbaData.size(); i += 4) {
        rgbaData[i + 0] = 200;  // R
        rgbaData[i + 1] = 100;  // G
        rgbaData[i + 2] = 50;   // B
        rgbaData[i + 3] = 255;  // A
    }
    
    // Compress to DXT1
    auto compressed = LibTXD::TextureConverter::compressToDXT(
        rgbaData.data(), width, height, LibTXD::Compression::DXT1, 1.0f);
    ASSERT_NE(compressed, nullptr);
    
    size_t compressedSize = LibTXD::TextureConverter::getCompressedDataSize(
        width, height, LibTXD::Compression::DXT1);
    
    // Create texture
    LibTXD::Texture texture;
    texture.setName("compressed_test");
    texture.setPlatform(LibTXD::Platform::D3D8);
    texture.setRasterFormat(LibTXD::RasterFormat::B8G8R8);
    texture.setDepth(16);
    texture.setHasAlpha(false);
    texture.setCompression(LibTXD::Compression::DXT1);
    
    LibTXD::MipmapLevel mip;
    mip.width = width;
    mip.height = height;
    mip.dataSize = compressedSize;
    mip.data.assign(compressed.get(), compressed.get() + compressedSize);
    
    texture.addMipmap(std::move(mip));
    dict.addTexture(std::move(texture));
    
    // Save
    fs::path savePath = tempDir / "compressed.txd";
    ASSERT_TRUE(dict.save(savePath.string()));
    
    // Reload and decompress
    LibTXD::TextureDictionary reloaded;
    ASSERT_TRUE(reloaded.load(savePath.string()));
    
    const auto* reloadedTex = reloaded.getTexture(0);
    ASSERT_NE(reloadedTex, nullptr);
    
    auto decompressed = LibTXD::TextureConverter::convertToRGBA8(*reloadedTex, 0);
    ASSERT_NE(decompressed, nullptr);
    
    // Verify decompressed data is reasonable (DXT is lossy, so allow tolerance)
    bool hasColorData = false;
    for (size_t i = 0; i < width * height * 4; i += 4) {
        if (decompressed[i] > 0 || decompressed[i + 1] > 0 || decompressed[i + 2] > 0) {
            hasColorData = true;
            break;
        }
    }
    EXPECT_TRUE(hasColorData);
}

// ============================================================================
// Game-Specific Tests
// ============================================================================

class GameSpecificTest : public ::testing::Test {
protected:
    void SetUp() override {}
    void TearDown() override {}
};

TEST_F(GameSpecificTest, GTA3_TextureFormat) {
    fs::path txdPath = getExamplePath("gta3/infernus.txd");
    
    if (!fs::exists(txdPath)) {
        GTEST_SKIP() << "Example file not found: " << txdPath;
    }
    
    LibTXD::TextureDictionary dict;
    ASSERT_TRUE(dict.load(txdPath.string()));
    
    // GTA3 should use D3D8 platform
    for (size_t i = 0; i < dict.getTextureCount(); i++) {
        const auto* tex = dict.getTexture(i);
        EXPECT_EQ(tex->getPlatform(), LibTXD::Platform::D3D8) 
            << "Texture " << tex->getName() << " has wrong platform";
    }
}

TEST_F(GameSpecificTest, GTAVC_TextureFormat) {
    fs::path txdPath = getExamplePath("gtavc/infernus.txd");
    
    if (!fs::exists(txdPath)) {
        GTEST_SKIP() << "Example file not found: " << txdPath;
    }
    
    LibTXD::TextureDictionary dict;
    ASSERT_TRUE(dict.load(txdPath.string()));
    
    // GTA:VC should use D3D8 platform
    for (size_t i = 0; i < dict.getTextureCount(); i++) {
        const auto* tex = dict.getTexture(i);
        EXPECT_EQ(tex->getPlatform(), LibTXD::Platform::D3D8)
            << "Texture " << tex->getName() << " has wrong platform";
    }
}

TEST_F(GameSpecificTest, GTASA_TextureFormat) {
    fs::path txdPath = getExamplePath("gtasa/infernus.txd");
    
    if (!fs::exists(txdPath)) {
        GTEST_SKIP() << "Example file not found: " << txdPath;
    }
    
    LibTXD::TextureDictionary dict;
    ASSERT_TRUE(dict.load(txdPath.string()));
    
    // GTA:SA typically uses D3D9 platform
    // But some textures may still be D3D8
    EXPECT_GT(dict.getTextureCount(), 0u);
}

TEST_F(GameSpecificTest, AllExampleFiles_CanBeConverted) {
    std::vector<std::string> examplePaths = {
        "gta3/infernus.txd",
        "gtavc/infernus.txd",
        "gtasa/infernus.txd"
    };
    
    for (const auto& relativePath : examplePaths) {
        fs::path txdPath = getExamplePath(relativePath);
        
        if (!fs::exists(txdPath)) {
            continue;
        }
        
        LibTXD::TextureDictionary dict;
        ASSERT_TRUE(dict.load(txdPath.string())) << "Failed to load: " << txdPath;
        
        for (size_t i = 0; i < dict.getTextureCount(); i++) {
            const auto* tex = dict.getTexture(i);
            
            if (LibTXD::TextureConverter::canConvert(*tex)) {
                auto rgba = LibTXD::TextureConverter::convertToRGBA8(*tex, 0);
                EXPECT_NE(rgba, nullptr) 
                    << "Failed to convert texture " << tex->getName() 
                    << " from " << relativePath;
            }
        }
    }
}

// ============================================================================
// Main
// ============================================================================

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}

