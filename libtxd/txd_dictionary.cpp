#include "txd_dictionary.h"
#include "txd_types.h"
#include <fstream>
#include <algorithm>
#include <cstring>

namespace LibTXD {

TextureDictionary::TextureDictionary()
    : version(0x1803FFFF)  // Default to SA
    , gameVersion(GameVersion::SA)
{
}

TextureDictionary::~TextureDictionary() = default;

TextureDictionary::TextureDictionary(TextureDictionary&& other) noexcept
    : textures(std::move(other.textures))
    , textureMap(std::move(other.textureMap))
    , version(other.version)
    , gameVersion(other.gameVersion)
{
}

TextureDictionary& TextureDictionary::operator=(TextureDictionary&& other) noexcept {
    if (this != &other) {
        textures = std::move(other.textures);
        textureMap = std::move(other.textureMap);
        version = other.version;
        gameVersion = other.gameVersion;
    }
    return *this;
}

Texture* TextureDictionary::getTexture(size_t index) {
    if (index >= textures.size()) {
        return nullptr;
    }
    return &textures[index];
}

const Texture* TextureDictionary::getTexture(size_t index) const {
    if (index >= textures.size()) {
        return nullptr;
    }
    return &textures[index];
}

Texture* TextureDictionary::findTexture(const std::string& name) {
    std::string lowerName = name;
    std::transform(lowerName.begin(), lowerName.end(), lowerName.begin(), ::tolower);
    
    auto it = textureMap.find(lowerName);
    if (it != textureMap.end()) {
        return &textures[it->second];
    }
    return nullptr;
}

const Texture* TextureDictionary::findTexture(const std::string& name) const {
    std::string lowerName = name;
    std::transform(lowerName.begin(), lowerName.end(), lowerName.begin(), ::tolower);
    
    auto it = textureMap.find(lowerName);
    if (it != textureMap.end()) {
        return &textures[it->second];
    }
    return nullptr;
}

void TextureDictionary::addTexture(Texture texture) {
    std::string lowerName = texture.getName();
    std::transform(lowerName.begin(), lowerName.end(), lowerName.begin(), ::tolower);
    
    textureMap[lowerName] = textures.size();
    textures.push_back(std::move(texture));
}

void TextureDictionary::removeTexture(size_t index) {
    if (index >= textures.size()) {
        return;
    }
    
    std::string lowerName = textures[index].getName();
    std::transform(lowerName.begin(), lowerName.end(), lowerName.begin(), ::tolower);
    textureMap.erase(lowerName);
    
    textures.erase(textures.begin() + index);
    rebuildTextureMap();
}

void TextureDictionary::removeTexture(const std::string& name) {
    std::string lowerName = name;
    std::transform(lowerName.begin(), lowerName.end(), lowerName.begin(), ::tolower);
    
    auto it = textureMap.find(lowerName);
    if (it != textureMap.end()) {
        removeTexture(it->second);
    }
}

void TextureDictionary::clear() {
    textures.clear();
    textureMap.clear();
}

bool TextureDictionary::load(const std::string& filepath) {
    std::ifstream file(filepath, std::ios::binary);
    if (!file.is_open()) {
        return false;
    }
    return load(file);
}

bool TextureDictionary::load(std::istream& stream) {
    clear();
    return readFromStream(stream);
}

bool TextureDictionary::save(const std::string& filepath) const {
    std::ofstream file(filepath, std::ios::binary);
    if (!file.is_open()) {
        return false;
    }
    return save(file);
}

bool TextureDictionary::save(std::ostream& stream) const {
    return writeToStream(stream);
}

bool TextureDictionary::readFromStream(std::istream& stream) {
    ChunkHeader header;
    if (!header.read(stream)) {
        return false;
    }
    
    if (header.type != ChunkType::TEXDICTIONARY) {
        return false;
    }
    
    version = header.version;
    gameVersion = detectGameVersion(version);
    
    size_t sectionStart = stream.tellg();
    size_t sectionEnd = sectionStart + header.length;
    
    // Read child sections
    while (stream.tellg() < static_cast<std::streampos>(sectionEnd) && stream.good()) {
        ChunkHeader childHeader;
        if (!childHeader.read(stream)) {
            break;
        }
        
        size_t childStart = stream.tellg();
        size_t childEnd = childStart + childHeader.length;
        
        if (childHeader.type == ChunkType::STRUCT) {
            // Read texture count
            uint16_t textureCount;
            stream.read(reinterpret_cast<char*>(&textureCount), 2);
            textureCount = fromLittleEndian16(textureCount);
            
            // Skip unknown field (2 bytes)
            stream.seekg(2, std::ios::cur);
            
            // Skip to end of struct
            stream.seekg(childEnd, std::ios::beg);
        } else if (childHeader.type == ChunkType::TEXTURENATIVE) {
            // The readD3D function expects to read the TEXTURENATIVE header first
            // But we've already read it, so we need to seek back
            stream.seekg(childStart - 12, std::ios::beg);
            
            Texture texture;
            if (texture.readD3D(stream)) {
                addTexture(std::move(texture));
            }
            // Ensure we're at the end of the section
            stream.seekg(childEnd, std::ios::beg);
        } else if (childHeader.type == ChunkType::EXTENSION) {
            // Skip extension section
            stream.seekg(childHeader.length, std::ios::cur);
        } else {
            // Unknown section, skip it
            stream.seekg(childHeader.length, std::ios::cur);
        }
    }
    
    return true;
}

bool TextureDictionary::writeToStream(std::ostream& stream) const {
    size_t sectionStart = stream.tellp();
    
    // Write TEXDICTIONARY header (will update later)
    ChunkHeader sectionHeader;
    sectionHeader.type = ChunkType::TEXDICTIONARY;
    sectionHeader.length = 0; // Will update
    sectionHeader.version = version;
    sectionHeader.write(stream);
    
    // Write STRUCT section (texture count)
    ChunkHeader structHeader;
    structHeader.type = ChunkType::STRUCT;
    structHeader.length = 4;
    structHeader.version = version;
    structHeader.write(stream);
    
    uint16_t textureCount = toLittleEndian16(static_cast<uint16_t>(textures.size()));
    stream.write(reinterpret_cast<const char*>(&textureCount), 2);
    
    // Write unknown field (2 bytes, typically 0)
    uint16_t unknown = 0;
    stream.write(reinterpret_cast<const char*>(&unknown), 2);
    
    // Write all textures
    for (const auto& texture : textures) {
        texture.writeD3D(stream);
    }
    
    // Write extension section (empty)
    ChunkHeader extHeader;
    extHeader.type = ChunkType::EXTENSION;
    extHeader.length = 0;
    extHeader.version = version;
    extHeader.write(stream);
    
    // Update section size
    size_t sectionEnd = stream.tellp();
    stream.seekp(sectionStart + 4, std::ios::beg);
    uint32_t sectionSize = toLittleEndian32(static_cast<uint32_t>(sectionEnd - sectionStart - 12));
    stream.write(reinterpret_cast<const char*>(&sectionSize), 4);
    stream.seekp(sectionEnd, std::ios::beg);
    
    return true;
}

GameVersion TextureDictionary::detectGameVersion(uint32_t versionValue) {
    uint16_t lower16 = static_cast<uint16_t>(versionValue & 0xFFFF);
    uint16_t upper16 = static_cast<uint16_t>((versionValue >> 16) & 0xFFFF);
    
    if (upper16 == 0x0C02 && lower16 == 0xFFFF) {
        return GameVersion::VC_PS2;
    } else if (upper16 == 0x1003 && lower16 == 0xFFFF) {
        return GameVersion::VC_PC;
    } else if (upper16 == 0x1803 && lower16 == 0xFFFF) {
        return GameVersion::SA;
    } else if (versionValue == 0x00000302 || versionValue == 0x00000304 || versionValue == 0x00000310) {
        return GameVersion::GTA3_1;
    } else if (versionValue == 0x0800FFFF) {
        return GameVersion::GTA3_4;
    }
    
    return GameVersion::UNKNOWN;
}

void TextureDictionary::rebuildTextureMap() {
    textureMap.clear();
    for (size_t i = 0; i < textures.size(); i++) {
        std::string lowerName = textures[i].getName();
        std::transform(lowerName.begin(), lowerName.end(), lowerName.begin(), ::tolower);
        textureMap[lowerName] = i;
    }
}

} // namespace LibTXD
