#pragma once
#include <array>
#include <cstdint>
#include <fstream>
#include <string>
#include <utility>
#include <vector>
#include "directory_entry.hpp"

namespace fat16 {

struct BPB {
    uint16_t bytesPerSector{}; // 0x0B
    uint8_t  sectorsPerCluster{}; // 0x0D
    uint16_t reservedSectors{}; // 0x0E
    uint8_t  numFATs{}; // 0x10
    uint16_t rootEntryCount{}; // 0x11
    uint16_t totalSectors16{}; // 0x13
    uint8_t  media{}; // 0x15
    uint16_t fatSize16{}; // 0x16
    uint16_t sectorsPerTrack{}; // 0x18
    uint16_t numHeads{}; // 0x1A
    uint32_t hiddenSectors{}; // 0x1C
    uint32_t totalSectors32{}; // 0x20
};

struct FileAttributes {
    bool readOnly{};
    bool hidden{};
    bool system{};
    FatDateTime creation{};
    FatDateTime modified{};
    uint32_t size{};
    std::string name;
};

class FAT16Image {
public:
    FAT16Image(const std::string& path, bool readWrite);

    // Info
    const BPB& bpb() const { return bpb_; }

    // Diretório raiz
    std::vector<std::pair<std::string, uint32_t>> list_root_files();
    DirectoryEntry get_entry_by_name(const std::string& name);
    int find_entry_index_by_name11(const std::array<char,11>& name11);
    int find_free_dir_index();

    // Arquivos
    std::vector<uint8_t> read_file_by_name(const std::string& name);
    FileAttributes get_attributes(const std::string& name);
    void rename_file(const std::string& oldName, const std::string& newName);
    void remove_file(const std::string& name);
    void add_file(const std::string& hostPath, const std::string& targetName);

    // FAT
    uint16_t read_fat(uint16_t cluster);
    void write_fat(uint16_t cluster, uint16_t value);
    std::vector<uint16_t> allocate_chain(size_t count);
    void free_chain(uint16_t firstCluster);

    // Dados
    std::vector<uint8_t> read_file_data(uint16_t firstCluster, uint32_t size);
    void write_file_data(const std::vector<uint8_t>& data, const std::vector<uint16_t>& chain);

private:
    void load_bpb_();
    uint32_t sector_of_cluster_(uint16_t clus) const;
    std::streamoff offset_of_sector_(uint32_t sector) const;
    std::streamoff offset_of_cluster_(uint16_t clus) const;
    uint32_t root_dir_offset_() const;
    size_t root_dir_bytes_() const;
    std::vector<DirectoryEntry> read_root_dir(std::vector<uint8_t>* rawOut);
    uint16_t find_free_cluster(uint16_t startFrom);
    void write_root_entry(size_t index, const DirectoryEntry& e);

    // estado
    std::string imagePath_;
    bool rw_{};
    std::fstream fs_;
    BPB bpb_{};
    uint32_t totalSectors_{};
    uint32_t rootDirSectors_{};
    uint32_t firstFATSector_{};
    uint32_t firstRootDirSector_{};
    uint32_t firstDataSector_{};
    uint32_t bytesPerCluster_{};
    uint32_t totalClusters_{};
};

} // namespace fat16
