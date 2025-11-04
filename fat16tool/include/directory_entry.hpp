#pragma once
#include <cstdint>
#include <string>
#include <array>
#include "utils.hpp"

namespace fat16 {

// Atributos de arquivo FAT
enum DirAttr : uint8_t {
    ATTR_READ_ONLY = 0x01,
    ATTR_HIDDEN    = 0x02,
    ATTR_SYSTEM    = 0x04,
    ATTR_VOLUME_ID = 0x08,
    ATTR_DIRECTORY = 0x10,
    ATTR_ARCHIVE   = 0x20,
    ATTR_LFN       = 0x0F // combinação para LFN
};

struct DirectoryEntry {
    std::array<uint8_t, 11> name{}; // 8.3
    uint8_t attr{};
    uint8_t ntRes{}; // reservado
    uint8_t crtTimeTenth{}; // 10ms
    uint16_t crtTime{}; // criação
    uint16_t crtDate{};
    uint16_t lastAccDate{};
    uint16_t firstClusHI{}; // FAT32
    uint16_t wrtTime{}; // última modificação
    uint16_t wrtDate{};
    uint16_t firstClusLO{}; // para FAT16
    uint32_t fileSize{};

    bool isLFN() const { return (attr & 0x0F) == ATTR_LFN; }
    bool isVolume() const { return (attr & ATTR_VOLUME_ID) != 0; }
    bool isDirectory() const { return (attr & ATTR_DIRECTORY) != 0; }
    bool isDeleted() const { return name[0] == 0xE5; }
    bool isUnused() const { return name[0] == 0x00; }
    bool isFile() const { return !isLFN() && !isVolume() && !isDirectory() && !isDeleted() && !isUnused(); }

    uint16_t firstCluster() const { return firstClusLO; }

    std::string displayName() const { return to_display_name(name.data()); }

    static DirectoryEntry parse(const uint8_t raw[32]);
    void serialize(uint8_t raw[32]) const;
};

} // namespace fat16
