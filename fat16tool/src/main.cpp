#include <iostream>
#include <iomanip>
#include <stdexcept>
#include <string>
#include <vector>
#include "fat16_image.hpp"
#include "utils.hpp"

using namespace fat16;

static void usage() {
    std::cerr << "Uso: fat16tool <imagem> <comando> [args]\n";
    std::cerr << "Comandos:\n"
                 "  list\n"
                 "  cat <ARQ>\n"
                 "  attrs <ARQ>\n"
                 "  rename <OLD> <NEW>\n"
                 "  add <CAMINHO_HOST> [NOME_8.3]\n"
                 "  rm <ARQ>\n";
}

static void print_time(const FatDateTime& dt) {
    auto t = to_time_t(dt);
    std::tm lt{};
#ifdef _WIN32
    localtime_s(&lt, &t);
#else
    localtime_r(&t, &lt);
#endif
    char buf[64];
    std::strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", &lt);
    std::cout << buf;
}

int main(int argc, char** argv) {
    if (argc < 3) {
        usage();
        return 1;
    }

    std::string img = argv[1];
    std::string cmd = argv[2];

    try {
        bool needsWrite = (cmd == "rename" || cmd == "rm" || cmd == "add");
        FAT16Image fs(img, needsWrite);

        if (cmd == "list") {
            auto files = fs.list_root_files();
            for (const auto& [name, size] : files) {
                std::cout << std::left << std::setw(20) << name << " " << size << " bytes\n";
            }
        } else if (cmd == "cat") {
            if (argc < 4) { usage(); return 1; }
            std::string name = argv[3];
            auto data = fs.read_file_by_name(name);
            std::cout.write(reinterpret_cast<const char*>(data.data()), static_cast<std::streamsize>(data.size()));
        } else if (cmd == "attrs") {
            if (argc < 4) { usage(); return 1; }
            std::string name = argv[3];
            auto a = fs.get_attributes(name);
            std::cout << "Nome: " << a.name << "\n";
            std::cout << "Tamanho: " << a.size << " bytes\n";
            std::cout << "Somente leitura: " << (a.readOnly ? "sim" : "não") << "\n";
            std::cout << "Oculto: " << (a.hidden ? "sim" : "não") << "\n";
            std::cout << "Sistema: " << (a.system ? "sim" : "não") << "\n";
            std::cout << "Criação: "; print_time(a.creation); std::cout << "\n";
            std::cout << "Modificação: "; print_time(a.modified); std::cout << "\n";
        } else if (cmd == "rename") {
            if (argc < 5) { usage(); return 1; }
            std::string oldN = argv[3];
            std::string newN = argv[4];
            fs.rename_file(oldN, newN);
            std::cout << "Renomeado com sucesso.\n";
        } else if (cmd == "rm") {
            if (argc < 4) { usage(); return 1; }
            std::string name = argv[3];
            fs.remove_file(name);
            std::cout << "Removido com sucesso.\n";
        } else if (cmd == "add") {
            if (argc < 4) { usage(); return 1; }
            std::string host = argv[3];
            std::string tgt = (argc >= 5) ? argv[4] : std::string();
            fs.add_file(host, tgt);
            std::cout << "Adicionado com sucesso.\n";
        } else {
            usage();
            return 1;
        }
    } catch (const std::exception& ex) {
        std::cerr << "Erro: " << ex.what() << "\n";
        return 2;
    }

    return 0;
}
