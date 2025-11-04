#include "fat16_image.hpp"
#include <algorithm>
#include <cstring>
#include <stdexcept>
#include <vector>
#include <iomanip>

namespace fat16 {

static void read_exact(std::fstream& fs, std::streamoff off, void* buf, std::size_t n) {
    fs.seekg(off);
    fs.read(reinterpret_cast<char*>(buf), static_cast<std::streamsize>(n));
    if (!fs) throw std::runtime_error("Falha ao ler da imagem");
}

static void write_exact(std::fstream& fs, std::streamoff off, const void* buf, std::size_t n) {
    fs.seekp(off);
    fs.write(reinterpret_cast<const char*>(buf), static_cast<std::streamsize>(n));
    if (!fs) throw std::runtime_error("Falha ao escrever na imagem");
    fs.flush();
}

DirectoryEntry DirectoryEntry::parse(const uint8_t raw[32]) {
    DirectoryEntry e{};
    std::memcpy(e.name.data(), raw + 0x00, 11);
    e.attr = raw[0x0B];
    e.ntRes = raw[0x0C];
    e.crtTimeTenth = raw[0x0D];
    e.crtTime = le16(raw + 0x0E);
    e.crtDate = le16(raw + 0x10);
    e.lastAccDate = le16(raw + 0x12);
    e.firstClusHI = le16(raw + 0x14);
    e.wrtTime = le16(raw + 0x16);
    e.wrtDate = le16(raw + 0x18);
    e.firstClusLO = le16(raw + 0x1A);
    e.fileSize = le32(raw + 0x1C);
    return e;
}

void DirectoryEntry::serialize(uint8_t raw[32]) const {
    std::memset(raw, 0, 32);
    std::memcpy(raw + 0x00, name.data(), 11);
    raw[0x0B] = attr;
    raw[0x0C] = ntRes;
    raw[0x0D] = crtTimeTenth;
    wr_le16(raw + 0x0E, crtTime);
    wr_le16(raw + 0x10, crtDate);
    wr_le16(raw + 0x12, lastAccDate);
    wr_le16(raw + 0x14, firstClusHI);
    wr_le16(raw + 0x16, wrtTime);
    wr_le16(raw + 0x18, wrtDate);
    wr_le16(raw + 0x1A, firstClusLO);
    wr_le32(raw + 0x1C, fileSize);
}

FAT16Image::FAT16Image(const std::string& path, bool readWrite)
    : imagePath_(path), rw_(readWrite) {
    std::ios::openmode mode = std::ios::binary | std::ios::in;
    if (rw_) mode |= std::ios::out;
    fs_.open(imagePath_, mode);
    if (!fs_) throw std::runtime_error("Não foi possível abrir a imagem: " + path);
    load_bpb_();
}

void FAT16Image::load_bpb_() {
    std::array<uint8_t, 512> boot{};
    read_exact(fs_, 0, boot.data(), boot.size());

    bpb_.bytesPerSector = le16(boot.data() + 0x0B);
    bpb_.sectorsPerCluster = boot[0x0D];
    bpb_.reservedSectors = le16(boot.data() + 0x0E);
    bpb_.numFATs = boot[0x10];
    bpb_.rootEntryCount = le16(boot.data() + 0x11);
    bpb_.totalSectors16 = le16(boot.data() + 0x13);
    bpb_.media = boot[0x15];
    bpb_.fatSize16 = le16(boot.data() + 0x16);
    bpb_.sectorsPerTrack = le16(boot.data() + 0x18);
    bpb_.numHeads = le16(boot.data() + 0x1A);
    bpb_.hiddenSectors = le32(boot.data() + 0x1C);
    bpb_.totalSectors32 = le32(boot.data() + 0x20);

    totalSectors_ = bpb_.totalSectors16 ? bpb_.totalSectors16 : bpb_.totalSectors32;

    rootDirSectors_ = ((bpb_.rootEntryCount * 32) + (bpb_.bytesPerSector - 1)) / bpb_.bytesPerSector;
    firstFATSector_ = bpb_.reservedSectors;
    firstRootDirSector_ = static_cast<uint32_t>(bpb_.reservedSectors + bpb_.numFATs * bpb_.fatSize16);
    firstDataSector_ = static_cast<uint32_t>(firstRootDirSector_ + rootDirSectors_);

    bytesPerCluster_ = static_cast<uint32_t>(bpb_.bytesPerSector) * bpb_.sectorsPerCluster;
    totalClusters_ = (totalSectors_ - firstDataSector_) / bpb_.sectorsPerCluster;

    // Verificação simples de tipo
    if (bpb_.fatSize16 == 0 || bpb_.rootEntryCount == 0) {
        throw std::runtime_error("Imagem não parece ser FAT16 válida");
    }
}

uint32_t FAT16Image::sector_of_cluster_(uint16_t clus) const {
    if (clus < 2) throw std::runtime_error("Cluster inválido (<2)");
    return firstDataSector_ + static_cast<uint32_t>(clus - 2) * bpb_.sectorsPerCluster;
}

std::streamoff FAT16Image::offset_of_sector_(uint32_t sector) const {
    return static_cast<std::streamoff>(sector) * bpb_.bytesPerSector;
}

std::streamoff FAT16Image::offset_of_cluster_(uint16_t clus) const {
    return offset_of_sector_(sector_of_cluster_(clus));
}

uint16_t FAT16Image::read_fat(uint16_t cluster) {
    std::array<uint8_t, 2> buf{};
    auto off = offset_of_sector_(firstFATSector_) + static_cast<std::streamoff>(cluster) * 2;
    read_exact(fs_, off, buf.data(), buf.size());
    return static_cast<uint16_t>(buf[0]) | (static_cast<uint16_t>(buf[1]) << 8);
}

void FAT16Image::write_fat(uint16_t cluster, uint16_t value) {
    if (!rw_) throw std::runtime_error("Imagem aberta como somente leitura");
    std::array<uint8_t, 2> buf{ static_cast<uint8_t>(value & 0xFF), static_cast<uint8_t>((value >> 8) & 0xFF) };
    for (int i = 0; i < bpb_.numFATs; ++i) {
        auto fatSector = firstFATSector_ + static_cast<uint32_t>(i) * bpb_.fatSize16;
        auto off = offset_of_sector_(fatSector) + static_cast<std::streamoff>(cluster) * 2;
        write_exact(fs_, off, buf.data(), buf.size());
    }
}

uint16_t FAT16Image::find_free_cluster(uint16_t startFrom) {
    if (startFrom < 2) startFrom = 2;
    for (uint32_t c = startFrom; c < totalClusters_ + 2; ++c) {
        if (read_fat(static_cast<uint16_t>(c)) == 0x0000) {
            return static_cast<uint16_t>(c);
        }
    }
    return 0; // nenhum livre
}

std::vector<uint16_t> FAT16Image::allocate_chain(size_t count) {
    if (!rw_) throw std::runtime_error("Imagem aberta como somente leitura");
    std::vector<uint16_t> chain;
    chain.reserve(count);
    uint16_t prev = 0;
    for (size_t i = 0; i < count; ++i) {
        uint16_t free = find_free_cluster(prev ? prev + 1 : 2);
        if (free == 0) {
            // rollback
            for (auto c : chain) write_fat(c, 0x0000);
            throw std::runtime_error("Sem espaço livre para alocar clusters");
        }
        write_fat(free, 0xFFF8); // marca provisoriamente como EOC
        if (prev != 0) write_fat(prev, free);
        chain.push_back(free);
        prev = free;
    }
    // marca último como EOC
    if (!chain.empty()) write_fat(chain.back(), 0xFFFF);
    return chain;
}

std::vector<uint8_t> FAT16Image::read_file_data(uint16_t firstCluster, uint32_t size) {
    std::vector<uint8_t> out;
    if (size == 0 || firstCluster == 0) return out;
    out.reserve(size);

    uint16_t c = firstCluster;
    while (c >= 0x0002 && c < 0xFFF8) {
        auto off = offset_of_cluster_(c);
        std::vector<uint8_t> buf(bytesPerCluster_);
        read_exact(fs_, off, buf.data(), buf.size());
        size_t toCopy = std::min<uint32_t>(static_cast<uint32_t>(buf.size()), size - static_cast<uint32_t>(out.size()));
        out.insert(out.end(), buf.begin(), buf.begin() + static_cast<long>(toCopy));
        if (out.size() >= size) break;
        c = read_fat(c);
    }
    // pode haver lixo no cluster final; já limitado por size
    return out;
}

void FAT16Image::write_file_data(const std::vector<uint8_t>& data, const std::vector<uint16_t>& chain) {
    if (!rw_) throw std::runtime_error("Imagem aberta como somente leitura");
    size_t written = 0;
    for (auto c : chain) {
        auto off = offset_of_cluster_(c);
        size_t toWrite = std::min<std::size_t>(bytesPerCluster_, data.size() - written);
        if (toWrite == 0) {
            // zera cluster extra
            std::vector<uint8_t> zero(bytesPerCluster_, 0);
            write_exact(fs_, off, zero.data(), zero.size());
        } else {
            write_exact(fs_, off, data.data() + static_cast<long>(written), toWrite);
            if (toWrite < bytesPerCluster_) {
                std::vector<uint8_t> zero(bytesPerCluster_ - toWrite, 0);
                write_exact(fs_, off + static_cast<std::streamoff>(toWrite), zero.data(), zero.size());
            }
        }
        written += toWrite;
        if (written >= data.size()) break;
    }
}

uint32_t FAT16Image::root_dir_offset_() const {
    return static_cast<uint32_t>(offset_of_sector_(firstRootDirSector_));
}

size_t FAT16Image::root_dir_bytes_() const {
    return static_cast<size_t>(rootDirSectors_) * bpb_.bytesPerSector;
}

std::vector<DirectoryEntry> FAT16Image::read_root_dir(std::vector<uint8_t>* rawOut) {
    auto size = root_dir_bytes_();
    std::vector<uint8_t> raw(size);
    read_exact(fs_, root_dir_offset_(), raw.data(), raw.size());
    if (rawOut) *rawOut = raw;

    std::vector<DirectoryEntry> entries;
    for (size_t i = 0; i + 32 <= raw.size(); i += 32) {
        DirectoryEntry e = DirectoryEntry::parse(&raw[i]);
        entries.push_back(e);
        if (e.isUnused()) {
            // Observação: entradas após 0x00 são livres segundo FAT, mas mantemos todas para edição
        }
    }
    return entries;
}

void FAT16Image::write_root_entry(size_t index, const DirectoryEntry& e) {
    if (!rw_) throw std::runtime_error("Imagem aberta como somente leitura");
    if (index * 32 >= root_dir_bytes_()) throw std::runtime_error("Índice de entrada inválido");
    std::array<uint8_t, 32> raw{};
    e.serialize(raw.data());
    write_exact(fs_, root_dir_offset_() + static_cast<std::streamoff>(index * 32), raw.data(), raw.size());
}

int FAT16Image::find_entry_index_by_name11(const std::array<char,11>& name11) {
    std::vector<uint8_t> raw;
    auto entries = read_root_dir(&raw);
    for (size_t i = 0; i < entries.size(); ++i) {
        const auto& e = entries[i];
        if (e.isLFN() || e.isVolume() || e.isDirectory() || e.isDeleted()) continue;
        if (std::memcmp(e.name.data(), name11.data(), 11) == 0) return static_cast<int>(i);
    }
    return -1;
}

int FAT16Image::find_free_dir_index() {
    std::vector<uint8_t> raw;
    auto entries = read_root_dir(&raw);
    for (size_t i = 0; i < entries.size(); ++i) {
        const auto& e = entries[i];
        if (e.isDeleted() || e.isUnused()) return static_cast<int>(i);
    }
    return -1;
}

void FAT16Image::free_chain(uint16_t firstCluster) {
    if (!rw_ || firstCluster == 0) return;
    uint16_t c = firstCluster;
    while (c >= 2 && c < 0xFFF8) {
        uint16_t next = read_fat(c);
        write_fat(c, 0x0000);
        if (next >= 0xFFF8) break;
        c = next;
    }
}

// Operações de alto nível
std::vector<std::pair<std::string, uint32_t>> FAT16Image::list_root_files() {
    auto entries = read_root_dir(nullptr);
    std::vector<std::pair<std::string, uint32_t>> out;
    for (const auto& e : entries) {
        if (e.isLFN() || e.isVolume() || e.isDirectory() || e.isDeleted() || e.isUnused()) continue;
        out.emplace_back(e.displayName(), e.fileSize);
    }
    return out;
}

DirectoryEntry FAT16Image::get_entry_by_name(const std::string& name) {
    auto n11 = make_83_name(name);
    int idx = find_entry_index_by_name11(n11);
    if (idx < 0) throw std::runtime_error("Arquivo não encontrado: " + name);
    std::vector<uint8_t> raw;
    auto entries = read_root_dir(&raw);
    return entries[static_cast<size_t>(idx)];
}

void FAT16Image::rename_file(const std::string& oldName, const std::string& newName) {
    auto old11 = make_83_name(oldName);
    auto new11 = make_83_name(newName);
    int oldIdx = find_entry_index_by_name11(old11);
    if (oldIdx < 0) throw std::runtime_error("Arquivo não encontrado: " + oldName);
    if (find_entry_index_by_name11(new11) >= 0) throw std::runtime_error("Já existe arquivo com este nome: " + newName);

    std::vector<uint8_t> raw;
    auto entries = read_root_dir(&raw);
    DirectoryEntry e = entries[static_cast<size_t>(oldIdx)];
    std::memcpy(e.name.data(), new11.data(), 11);

    write_root_entry(static_cast<size_t>(oldIdx), e);
}

void FAT16Image::remove_file(const std::string& name) {
    auto n11 = make_83_name(name);
    int idx = find_entry_index_by_name11(n11);
    if (idx < 0) throw std::runtime_error("Arquivo não encontrado: " + name);

    std::vector<uint8_t> raw;
    auto entries = read_root_dir(&raw);
    DirectoryEntry e = entries[static_cast<size_t>(idx)];

    // libera cadeia
    if (e.firstCluster() != 0) free_chain(e.firstCluster());

    // marca deletado
    e.name[0] = 0xE5;
    write_root_entry(static_cast<size_t>(idx), e);
}

void FAT16Image::add_file(const std::string& hostPath, const std::string& targetName) {
    // carrega arquivo do host
    std::ifstream in(hostPath, std::ios::binary);
    if (!in) throw std::runtime_error("Não foi possível abrir arquivo local: " + hostPath);
    std::vector<uint8_t> data((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());

    auto n11 = make_83_name(targetName.empty() ? hostPath : targetName);

    if (find_entry_index_by_name11(n11) >= 0) {
        throw std::runtime_error("Já existe arquivo com este nome no diretório raiz");
    }

    int freeIdx = find_free_dir_index();
    if (freeIdx < 0) throw std::runtime_error("Diretório raiz cheio");

    DirectoryEntry e{};
    std::memcpy(e.name.data(), n11.data(), 11);
    e.attr = ATTR_ARCHIVE; // arquivo normal

    // timestamps
    auto now = std::time(nullptr);
    auto fat = from_time_t(now);
    e.crtTimeTenth = fat.tenth;
    e.crtTime = fat.time;
    e.crtDate = fat.date;
    e.lastAccDate = fat.date;
    e.wrtTime = fat.time;
    e.wrtDate = fat.date;

    e.fileSize = static_cast<uint32_t>(data.size());

    if (data.empty()) {
        e.firstClusLO = 0; // arquivos vazios podem ter cluster 0
        write_root_entry(static_cast<size_t>(freeIdx), e);
        return;
    }

    // clusters necessários
    size_t clusters = (data.size() + bytesPerCluster_ - 1) / bytesPerCluster_;
    auto chain = allocate_chain(clusters);
    e.firstClusLO = chain.front();

    write_file_data(data, chain);
    write_root_entry(static_cast<size_t>(freeIdx), e);
}

std::vector<uint8_t> FAT16Image::read_file_by_name(const std::string& name) {
    auto e = get_entry_by_name(name);
    return read_file_data(e.firstCluster(), e.fileSize);
}

FileAttributes FAT16Image::get_attributes(const std::string& name) {
    auto e = get_entry_by_name(name);
    FileAttributes a{};
    a.readOnly = (e.attr & ATTR_READ_ONLY) != 0;
    a.hidden   = (e.attr & ATTR_HIDDEN) != 0;
    a.system   = (e.attr & ATTR_SYSTEM) != 0;
    a.creation = { e.crtDate, e.crtTime, e.crtTimeTenth };
    a.modified = { e.wrtDate, e.wrtTime, 0 };
    a.size     = e.fileSize;
    a.name     = e.displayName();
    return a;
}

} // namespace fat16
