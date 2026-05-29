> [English version](tutorial.md)

# CodeBrowser — Tutorial de Uso

CodeBrowser é uma ferramenta de navegação de código via browser, inspirada no Sourcetrail e OpenGrok.
Navegue pelo seu projeto de qualquer browser ou dispositivo móvel — sem IDE.

---

## Índice

1. [Pré-requisitos](#1-pré-requisitos)
2. [Build](#2-build)
3. [Configuração](#3-configuração)
4. [Rodando](#4-rodando)
5. [Home / Busca](#5-home--busca)
6. [Visualização de Pastas](#6-visualização-de-pastas)
7. [Editor de Código](#7-editor-de-código)
8. [Painel de Símbolos](#8-painel-de-símbolos)
9. [Navegação de Código](#9-navegação-de-código)
10. [Histórico e Blame](#10-histórico-e-blame)
11. [Settings — Seletor de Backend](#11-settings--seletor-de-backend)
12. [Grafos C++ com cxxlsp](#12-grafos-c-com-cxxlsp)

---

## 1. Pré-requisitos

| Ferramenta | Uso | Instalação |
|-----------|-----|-----------|
| `g++` / `clang++` | Build do CodeBrowser | `apt install g++` |
| `cmake >= 3.16` | Sistema de build | `apt install cmake` |
| `git` | Histórico de revisões | `apt install git` |
| `ctags` | Navegação de símbolos (backend ctags) | `apt install universal-ctags` |
| `clangd` | Análise semântica C++ (opcional) | `apt install clangd` |
| `cxxlsp` + `cxxidx` | Grafos de chamadas/herança C++ (opcional) | Build a partir de [cxx-index](../../cxx-index) |

---

## 2. Build

```bash
git clone https://github.com/gwerners/CodeBrowser
cd CodeBrowser
./run.bash noFullTextIndex   # build sem Apache Lucy (recomendado)
# ou
./run.bash                   # build com busca full-text (requer Clownfish/Lucy)
```

O script compila tudo e sobe o servidor em `http://localhost:3000`.

Para buildar sem iniciar o servidor:

```bash
mkdir -p build && cd build
cmake .. -DUSE_LUCY=OFF
make -j$(nproc)
cd ..
```

---

## 3. Configuração

Edite o `config.json` na raiz do projeto:

```json
{
    "symbol-provider": "ctags",
    "search-engine": "grep",
    "theme": "dark",
    "projects": [
        {
            "name": "meuprojeto",
            "source": "/caminho/absoluto/meuprojeto",
            "index":  "index/meuprojeto"
        }
    ]
}
```

### Opções de `symbol-provider`

| Valor | Requisito | Funcionalidades |
|-------|-----------|----------------|
| `"ctags"` | binário `ctags` | Definição, referências, hover, símbolos |
| `"clangd"` | `clangd` + `compile_commands.json` | Análise semântica completa |
| `"cxxlsp"` | `cxxlsp` + `index.cxxi` | C++ pré-indexado com grafos de chamada/herança |

### Configuração completa com cxxlsp

```json
{
    "symbol-provider": "cxxlsp",
    "lsp": "/caminho/para/cxxlsp",
    "cxxidx": "/caminho/para/cxxidx",
    "theme": "dark",
    "projects": [
        {
            "name": "meuprojeto",
            "source": "/caminho/para/meuprojeto",
            "index":  "index/meuprojeto",
            "cxxindex": "/caminho/para/meuprojeto/index.cxxi",
            "compilation-db": "/caminho/para/meuprojeto/build/compile_commands.json"
        }
    ]
}
```

---

## 4. Rodando

```bash
./build/work/codebrowser           # usa o config.json padrão
./build/work/codebrowser meu.json  # usa um config personalizado
```

Abra `http://localhost:3000` no browser.

---

## 5. Home / Busca

![Página de busca](images/search.png)

A página inicial permite buscar em todos os projetos configurados.

- **Full Search** — busca por qualquer string no conteúdo dos arquivos (grep ou Apache Lucy)
- **RegExp (filter)** — filtra os resultados por caminho de arquivo com uma expressão regular
- **Seletor de projeto** — escolha quais projetos pesquisar; duplo-clique no nome de um projeto para navegar pela árvore de pastas

---

## 6. Visualização de Pastas

![Visualização de pastas](images/folders.png)

Navegue pela árvore de diretórios do projeto. Clique em qualquer diretório para entrar nele, ou em um arquivo para abri-lo no editor. O link **History** mostra o histórico de commits do git para a pasta atual.

---

## 7. Editor de Código

![Editor](images/editor.png)

O editor usa Monaco (o mesmo motor do VS Code) em modo somente leitura com highlight de sintaxe.

**Links do navbar:**

| Link | Ação |
|------|------|
| Home | Voltar para a página de busca |
| History | Histórico de commits do git para este arquivo |
| Annotate | Git blame — mostra autor e data por linha |
| Symbols | Abrir/fechar o painel de símbolos |
| Settings | Abrir o modal de configurações de backend |

**Atalhos no editor:**

| Atalho | Ação |
|--------|------|
| `Ctrl+Clique` | Ir para a definição |
| `Shift+Ctrl+Clique` | Ir para a declaração |
| Clique direito | Abrir menu de contexto com todas as ações de navegação |

---

## 8. Painel de Símbolos

Clique em **Symbols** no navbar para abrir o painel de símbolos à esquerda.

Os símbolos são agrupados por tipo (funções, classes, métodos, campos…). Clique em qualquer símbolo para saltar para a linha correspondente no editor. O painel pode ser redimensionado arrastando a borda direita.

---

## 9. Navegação de Código

Clique com o botão direito em qualquer identificador no editor para abrir o menu de contexto:

| Ação | Descrição |
|------|-----------|
| Go to Definition | Ir para onde o símbolo está definido |
| Go to Declaration | Ir para a declaração forward |
| Go to Implementation | Ir para a função/método que implementa |
| Go to Type Definition | Ir para a definição do tipo |
| Show References | Overlay com todas as referências clicáveis |
| Switch Header/Source | Alternar entre `.h` e `.cpp` |
| Type Hierarchy | Navegar na hierarquia de tipos |
| Search Word | Busca full-text da palavra selecionada |
| **Show Callers** | Grafo de quem chama esta função *(somente cxxlsp)* |
| **Show Callees** | Grafo do que esta função chama *(somente cxxlsp)* |
| **Show Base Classes** | Grafo de herança para cima *(somente cxxlsp)* |
| **Show Derived Classes** | Grafo de herança para baixo *(somente cxxlsp)* |
| **Show Members** | Membros da classe/namespace *(somente cxxlsp)* |

---

## 10. Histórico e Blame

### Histórico do arquivo

Clique em **History** no editor para ver todos os commits que modificaram o arquivo atual. Clique num hash para ver o arquivo naquela revisão, ou selecione duas revisões com os botões de rádio e clique em **Compare** para ver um diff lado a lado.

### Annotate (Git Blame)

Clique em **Annotate** no editor para ver o git blame. Cada linha mostra o hash do commit, o autor e a data. Clique num hash para navegar para aquela versão do arquivo.

![Histórico](images/history.png)

---

## 11. Settings — Seletor de Backend

Clique em **Settings** no navbar do editor para abrir o modal de configurações.

**Seção Backend** mostra os três backends com badges de disponibilidade:

- `available` (verde) — binário encontrado no PATH ou no caminho configurado
- `not found` (vermelho) — binário não encontrado
- `active` (dourado) — backend atualmente ativo

> Para mudar o backend ativo, edite o `config.json` e reinicie o servidor.

**Seção C++ Index** (visível quando `symbol-provider` é `cxxlsp`):

- Mostra o número de nós, arestas e ocorrências no índice atual
- Botão **Re-index Now** — dispara o `cxxidx index` em background;
  um log de progresso aparece; o botão fica desabilitado enquanto a indexação roda

---

## 12. Grafos C++ com cxxlsp

O painel de grafo aparece à direita do editor quando você seleciona uma ação de grafo no menu de contexto.

![Editor com código](images/editor_header.png)

### Configurando o cxxlsp

**Passo 1 — Build do cxx-index:**

```bash
cd cxx-index
cmake -S . -B build -DLLVM_DIR=/usr/lib/llvm-19/lib/cmake/llvm \
                     -DClang_DIR=/usr/lib/llvm-19/lib/cmake/clang
cmake --build build --parallel
```

**Passo 2 — Gerar o compile_commands.json do seu projeto:**

```bash
cd /seu/projeto
cmake -DCMAKE_EXPORT_COMPILE_COMMANDS=ON -B build
```

**Passo 3 — Indexar o projeto:**

```bash
/caminho/para/cxx-index/build/cxxidx index build/compile_commands.json \
    -o /seu/projeto/index.cxxi -v
```

**Passo 4 — Configurar o CodeBrowser:**

```json
{
    "symbol-provider": "cxxlsp",
    "lsp": "/caminho/para/cxx-index/build/cxxlsp",
    "cxxidx": "/caminho/para/cxx-index/build/cxxidx",
    "projects": [{
        "name": "meuprojeto",
        "source": "/seu/projeto",
        "index":  "index/meuprojeto",
        "cxxindex": "/seu/projeto/index.cxxi"
    }]
}
```

### Usando o painel de grafo

1. Clique direito em um nome de função → **Show Callers** (ou Callees, Members, etc.)
2. O painel de grafo abre à direita
3. Cada nó tem uma cor conforme o tipo do símbolo:
   - Azul — função
   - Verde — método / construtor
   - Laranja — classe
   - Amarelo — struct
   - Roxo — enum
4. **Clique simples** em um nó → mostra nome completo e localização no tooltip
5. **Duplo-clique** em um nó → navega o browser para o arquivo e linha daquele símbolo
6. Arraste a borda esquerda do painel para redimensioná-lo
7. Clique em **×** no cabeçalho do grafo para fechar

### Reindexar após mudanças no código

Use o botão **Settings → Re-index Now**, ou rode manualmente:

```bash
cxxidx index build/compile_commands.json -o index.cxxi
```

O servidor usa o novo índice automaticamente na próxima requisição.
