#include "txd_types.h"
#include <istream>
#include <ostream>
#include <cstring>

namespace LibTXD {

bool ChunkHeader::read(std::istream& stream) {
    uint32_t typeVal, lengthVal, versionVal;
    
    stream.read(reinterpret_cast<char*>(&typeVal), 4);
    if (stream.gcount() != 4) {
        return false;
    }
    
    stream.read(reinterpret_cast<char*>(&lengthVal), 4);
    if (stream.gcount() != 4) {
        return false;
    }
    
    stream.read(reinterpret_cast<char*>(&versionVal), 4);
    if (stream.gcount() != 4) {
        return false;
    }
    
    type = static_cast<ChunkType>(fromLittleEndian32(typeVal));
    length = fromLittleEndian32(lengthVal);
    version = fromLittleEndian32(versionVal);
    
    return true;
}

uint32_t ChunkHeader::write(std::ostream& stream) const {
    uint32_t typeVal = toLittleEndian32(static_cast<uint32_t>(type));
    uint32_t lengthVal = toLittleEndian32(length);
    uint32_t versionVal = toLittleEndian32(version);
    
    stream.write(reinterpret_cast<const char*>(&typeVal), 4);
    stream.write(reinterpret_cast<const char*>(&lengthVal), 4);
    stream.write(reinterpret_cast<const char*>(&versionVal), 4);
    
    return 12;
}

} // namespace LibTXD
