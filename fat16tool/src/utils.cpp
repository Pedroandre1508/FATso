#include "utils.hpp"
#include <algorithm>
#include <array>
#include <cctype>
#include <cstring>
#include <stdexcept>

namespace fat16 {

FatDateTime from_time_t(std::time_t t) {
    std::tm lt{};
#ifdef _WIN32
    localtime_s(&lt, &t);
#else
    localtime_r(&t, &lt);
#endif
    FatDateTime dt{};
    int year = lt.tm_year + 1900;
    if (year < 1980) year = 1980; // FAT não representa antes de 1980
    dt.date = static_cast<uint16_t>(((year - 1980) & 0x7F) << 9 |
                                    ((lt.tm_mon + 1) & 0x0F) << 5 |
                                    (lt.tm_mday & 0x1F));
    dt.time = static_cast<uint16_t>(((lt.tm_hour) & 0x1F) << 11 |
                                    ((lt.tm_min) & 0x3F) << 5 |
                                    ((lt.tm_sec / 2) & 0x1F));
    dt.tenth = 0;
    return dt;
}

std::time_t to_time_t(const FatDateTime& dt) {
    std::tm lt{};
    lt.tm_year = ((dt.date >> 9) & 0x7F) + 80; // desde 1900
    lt.tm_mon  = ((dt.date >> 5) & 0x0F) - 1;
    lt.tm_mday = (dt.date & 0x1F);
    lt.tm_hour = (dt.time >> 11) & 0x1F;
    lt.tm_min  = (dt.time >> 5) & 0x3F;
    lt.tm_sec  = (dt.time & 0x1F) * 2;
    lt.tm_isdst = -1;
    return std::mktime(&lt);
}

static bool is_valid_83_char(unsigned char c) {
    // Aceita A-Z, 0-9 e caracteres permitidos no FAT 8.3 simples
    const char* allowed = "$%'-_@~`!(){}^#&";
    return (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') ||
           std::strchr(allowed, c) != nullptr;
}

std::array<char, 11> make_83_name(const std::string& input) {
    // Normaliza para upper e 8.3, removendo espaços e pontos extras
    // Estratégia simples: pega basename, separa na primeira '.', corta tamanhos
    std::string base = input;
    // remove path
    auto pos = base.find_last_of("/");
#ifdef _WIN32
    auto pos2 = base.find_last_of("\\");
    if (pos2 != std::string::npos) pos = std::max(pos, pos2);
#endif
    if (pos != std::string::npos) base = base.substr(pos + 1);

    // upper
    std::string up;
    up.reserve(base.size());
    for (unsigned char c : base) up.push_back(static_cast<char>(std::toupper(c)));

    // separa nome e extensão (primeiro '.')
    std::string name = up;
    std::string ext;
    auto dot = up.find('.');
    if (dot != std::string::npos) {
        name = up.substr(0, dot);
        ext = up.substr(dot + 1);
    }

    // filtra caracteres inválidos substituindo por '_'
    auto filter = [](std::string s) {
        for (auto& ch : s) {
            unsigned char c = static_cast<unsigned char>(ch);
            if (!is_valid_83_char(c)) ch = '_';
        }
        return s;
    };
    name = filter(name);
    ext = filter(ext);

    if (name.empty()) name = "_";

    if (name.size() > 8) name = name.substr(0, 8);
    if (ext.size() > 3) ext = ext.substr(0, 3);

    std::array<char, 11> out{};
    std::fill(out.begin(), out.end(), ' ');
    std::memcpy(out.data(), name.data(), name.size());
    if (!ext.empty()) std::memcpy(out.data() + 8, ext.data(), ext.size());
    return out;
}

std::string to_display_name(const uint8_t name11[11]) {
    std::string name(reinterpret_cast<const char*>(name11), 8);
    std::string ext(reinterpret_cast<const char*>(name11 + 8), 3);

    // trim right spaces
    auto rtrim = [](std::string& s) {
        while (!s.empty() && s.back() == ' ') s.pop_back();
    };
    rtrim(name);
    rtrim(ext);

    if (name.empty()) return std::string();
    if (!ext.empty()) return name + "." + ext;
    return name;
}

std::vector<std::vector<uint8_t>> chunk(const std::vector<uint8_t>& data, size_t chunkSize) {
    std::vector<std::vector<uint8_t>> out;
    if (chunkSize == 0) return out;
    size_t i = 0;
    while (i < data.size()) {
        size_t n = std::min(chunkSize, data.size() - i);
        out.emplace_back(data.begin() + static_cast<long>(i), data.begin() + static_cast<long>(i + n));
        i += n;
    }
    return out;
}

} // namespace fat16
