# Trabalho: Implementação de Funções de Manipulação de Sistema de Arquivos FAT16

## Objetivo

Desenvolver um programa para manipular um sistema de arquivos FAT16. O objetivo principal é implementar operações básicas de gerenciamento de arquivos, permitindo criação, leitura, escrita e remoção de arquivos.

Observação: por simplicidade, não considerar a criação e manipulação de subdiretórios (somente diretório raiz).

## Requisitos

Todas as operações devem ser realizadas sobre um arquivo que contenha uma imagem de disco formatada como FAT16. O programa deve ser capaz de:
- Ler a imagem,
- Interpretar sua estrutura,
- Realizar as operações de acordo com as especificações do sistema de arquivos FAT16.

A solução deve funcionar para qualquer arquivo contendo imagem de disco no formato FAT16, desconsiderando subdiretórios.

### Operações fundamentais

- Listar o conteúdo do disco: exibir os nomes dos arquivos e seus respectivos tamanhos existentes no diretório raiz.
- Listar o conteúdo de um arquivo: mostrar o conteúdo de um arquivo do diretório raiz.
- Exibir os atributos de um arquivo: mostrar data/hora de criação/última modificação e os atributos:
  - Somente leitura,
  - Oculto,
  - Arquivo de sistema.
- Renomear um arquivo: trocar o nome de um arquivo existente.
- Inserir/criar um novo arquivo: armazenar no diretório raiz um novo arquivo externo.
- Apagar/remover um arquivo: apagar um arquivo do diretório raiz.

## Avaliação

A avaliação será baseada nos seguintes critérios:

### Número de operações implementadas (pesos)

- Listar o conteúdo do disco — 10%
- Listar o conteúdo de um arquivo — 10%
- Exibir os atributos de um arquivo — 10%
- Renomear um arquivo — 10%
- Apagar/remover um arquivo — 20%
- Inserir/criar um novo arquivo — 40%

### Correção

Cada operação será avaliada de acordo com a sua corretude, seguindo as especificações do sistema de arquivos FAT16.