#ifndef TXD_DICTIONARY_H
#define TXD_DICTIONARY_H

#include "txd_texture.h"
#include "txd_types.h"
#include <cstdint>
#include <string>
#include <vector>
#include <memory>
#include <iosfwd>
#include <unordered_map>

namespace LibTXD {

// Texture Dictionary class - represents a TXD file
class TextureDictionary {
public:
    TextureDictionary();
    ~TextureDictionary();
    
    // Non-copyable, movable
    TextureDictionary(const TextureDictionary&) = delete;
    TextureDictionary& operator=(const TextureDictionary&) = delete;
    TextureDictionary(TextureDictionary&&) noexcept;
    TextureDictionary& operator=(TextureDictionary&&) noexcept;
    
    // Texture access
    size_t getTextureCount() const { return textures.size(); }
    Texture* getTexture(size_t index);
    const Texture* getTexture(size_t index) const;
    Texture* findTexture(const std::string& name);
    const Texture* findTexture(const std::string& name) const;
    
    // Texture management
    void addTexture(Texture texture);
    void removeTexture(size_t index);
    void removeTexture(const std::string& name);
    void clear();
    
    // Version info
    GameVersion getGameVersion() const { return gameVersion; }
    uint32_t getVersion() const { return version; }
    void setVersion(uint32_t v) { version = v; }
    
    // File I/O
    bool load(const std::string& filepath);
    bool load(std::istream& stream);
    bool save(const std::string& filepath) const;
    bool save(std::ostream& stream) const;
    
private:
    std::vector<Texture> textures;
    std::unordered_map<std::string, size_t> textureMap; // name -> index
    uint32_t version;
    GameVersion gameVersion;
    
    // Helper functions
    bool readFromStream(std::istream& stream);
    bool writeToStream(std::ostream& stream) const;
    GameVersion detectGameVersion(uint32_t versionValue);
    void rebuildTextureMap();
};

} // namespace LibTXD

#endif // TXD_DICTIONARY_H
