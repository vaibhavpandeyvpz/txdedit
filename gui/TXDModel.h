#ifndef TXD_MODEL_H
#define TXD_MODEL_H

#include <QObject>
#include <QString>
#include <QPixmap>
#include <vector>
#include <memory>
#include "libtxd/txd_types.h"

// Forward declarations
namespace LibTXD {
    class TextureDictionary;
}

// Simple texture entry - just holds data for presentation
struct TXDFileEntry {
    // Metadata
    QString name;
    QString maskName;
    LibTXD::RasterFormat rasterFormat;  // Original format (informational only, recalculated on save)
    bool compressionEnabled;  // Just a flag - compression happens on save
    uint32_t width;
    uint32_t height;
    bool hasAlpha;
    uint32_t mipmapCount;
    uint32_t filterFlags;
    bool isNew;  // Track if added by user or loaded from file
    
    // Platform info (D3D8 for GTA3/VC, D3D9 for SA)
    LibTXD::Platform platform;
    
    // Uncompressed data for display and editing (always RGBA8888)
    std::vector<uint8_t> diffuse;  // RGB + Alpha (if hasAlpha is true, alpha channel is meaningful)
    
    // Helper: Get combined RGBA (for preview)
    std::vector<uint8_t> getRGBA() const {
        return diffuse;
    }
    
    // Helper: Get RGB only (for diffuse view)
    std::vector<uint8_t> getRGB() const {
        std::vector<uint8_t> rgb;
        rgb.reserve(width * height * 3);
        for (size_t i = 0; i < diffuse.size(); i += 4) {
            rgb.push_back(diffuse[i]);     // R
            rgb.push_back(diffuse[i + 1]); // G
            rgb.push_back(diffuse[i + 2]); // B
        }
        return rgb;
    }
    
    // Helper: Get alpha channel only
    std::vector<uint8_t> getAlpha() const {
        std::vector<uint8_t> alpha;
        alpha.reserve(width * height);
        for (size_t i = 3; i < diffuse.size(); i += 4) {
            alpha.push_back(diffuse[i]);
        }
        return alpha;
    }
};

// Simple model - just holds data
class TXDModel : public QObject {
    Q_OBJECT

public:
    TXDModel(QObject* parent = nullptr);
    ~TXDModel();

    // File operations
    bool loadFromFile(const QString& filepath);
    bool saveToFile(const QString& filepath) const;
    void clear();

    // Metadata
    LibTXD::GameVersion getGameVersion() const { return gameVersion; }
    uint32_t getVersion() const { return version; }
    bool isModified() const { return modified; }
    QString getFilePath() const { return filePath; }
    void setVersion(uint32_t v) { version = v; setModified(true); }
    void setGameVersion(LibTXD::GameVersion gv) { gameVersion = gv; }

    // Texture access
    size_t getTextureCount() const { return entries.size(); }
    TXDFileEntry* getTexture(size_t index);
    const TXDFileEntry* getTexture(size_t index) const;
    TXDFileEntry* findTexture(const QString& name);
    const TXDFileEntry* findTexture(const QString& name) const;

    // Texture management
    void addTexture(TXDFileEntry entry);
    void removeTexture(size_t index);
    void removeTexture(const QString& name);

    // Model state
    void setModified(bool modified);
    void setFilePath(const QString& path);

signals:
    void textureAdded(size_t index);
    void textureRemoved(size_t index);
    void textureUpdated(size_t index);
    void modelChanged();
    void modifiedChanged(bool modified);

private:
    // Load from LibTXD::TextureDictionary - decompress immediately
    bool loadFromDictionary(LibTXD::TextureDictionary* dict);
    // Save to LibTXD::TextureDictionary - compress on-the-fly
    std::unique_ptr<LibTXD::TextureDictionary> createDictionary() const;

    std::vector<TXDFileEntry> entries;
    LibTXD::GameVersion gameVersion;
    uint32_t version;
    bool modified;
    QString filePath;
};

#endif // TXD_MODEL_H
