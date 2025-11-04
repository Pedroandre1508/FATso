# FAT16 Tool (C++)

Ferramenta em C++ para manipulação de imagens de disco FAT16 (sem subdiretórios).

Operações suportadas:
- list: lista arquivos do diretório raiz (nome e tamanho)
- cat <ARQ>: imprime o conteúdo de um arquivo
- attrs <ARQ>: mostra atributos, data/hora de criação e modificação
- rename <OLD> <NEW>: renomeia arquivo (nomes 8.3)
- add <CAMINHO_HOST> [NOME_8.3]: adiciona um novo arquivo ao diretório raiz
- rm <ARQ>: remove arquivo do diretório raiz

Uso:

```
# Build (Makefile)
make -C . -j

# Execução
./build/bin/fat16tool <imagem_fat16> <comando> [args]

# Exemplos
./build/bin/fat16tool disco.img list
./build/bin/fat16tool disco.img cat README.TXT
./build/bin/fat16tool disco.img attrs KERNEL.SYS
./build/bin/fat16tool disco.img rename OLDNAME.TXT NEWNAME.TXT
./build/bin/fat16tool disco.img add /caminho/arquivo.txt ARQTXT.TXT
./build/bin/fat16tool disco.img rm ARQTXT.TXT
```

Limitações e observações:
- Suporte a nomes no formato 8.3 (LFN é ignorado na listagem e não é criado)
- Apenas diretório raiz (sem subdiretórios)
- A imagem deve ser FAT16 válida
- Horários usam timezone local da máquina ao inserir arquivos
