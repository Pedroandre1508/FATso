#pragma once
#include <cstdint>
#include <string>
#include <vector>
#include <ctime>
#include <array>

namespace fat16 {

// Leitura little-endian
inline uint16_t le16(const uint8_t* p) {
    return static_cast<uint16_t>(p[0]) | (static_cast<uint16_t>(p[1]) << 8);
}

inline uint32_t le32(const uint8_t* p) {
    return static_cast<uint32_t>(p[0]) |
           (static_cast<uint32_t>(p[1]) << 8) |
           (static_cast<uint32_t>(p[2]) << 16) |
           (static_cast<uint32_t>(p[3]) << 24);
}

inline void wr_le16(uint8_t* p, uint16_t v) {
    p[0] = static_cast<uint8_t>(v & 0xFF);
    p[1] = static_cast<uint8_t>((v >> 8) & 0xFF);
}

inline void wr_le32(uint8_t* p, uint32_t v) {
    p[0] = static_cast<uint8_t>(v & 0xFF);
    p[1] = static_cast<uint8_t>((v >> 8) & 0xFF);
    p[2] = static_cast<uint8_t>((v >> 16) & 0xFF);
    p[3] = static_cast<uint8_t>((v >> 24) & 0xFF);
}

// Conversões de data/hora FAT
struct FatDateTime {
    uint16_t date{}; // bits: Y-1980 (15..9), M (8..5), D (4..0)
    uint16_t time{}; // bits: h (15..11), m (10..5), s/2 (4..0)
    uint8_t tenth{}; // 10ms (opcional)
};

FatDateTime from_time_t(std::time_t t);
std::time_t to_time_t(const FatDateTime& dt);

// Nomes 8.3 em uppercase, preenchidos com espaços
// Retorna string de 11 bytes (8 nome + 3 ext) ou lança std::runtime_error
std::array<char, 11> make_83_name(const std::string& input);

// Converte 11 bytes para string "NAME.EXT" (sem espaços)
std::string to_display_name(const uint8_t name11[11]);

// Utilitário: reparte bytes de um buffer em blocos de tamanho fixo
std::vector<std::vector<uint8_t>> chunk(const std::vector<uint8_t>& data, size_t chunkSize);

} // namespace fat16
