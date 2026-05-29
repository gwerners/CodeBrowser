# Indexing a Project in CodeBrowser

This guide explains how to index a C/C++ project so that CodeBrowser can provide
deep navigation features: call graphs, inheritance hierarchies, member lists, and
go-to-definition powered by **cxx-index**.

---

## Prerequisites

- `cxxidx` built from the [cxx-index](../cxx-index) project
- A C/C++ project with a `compile_commands.json` (see step 1)

---

## Step 1 — Generate `compile_commands.json`

The indexer needs a compilation database to know how each file is compiled.

**CMake projects (recommended):**
```bash
cd /path/to/your/project
cmake -DCMAKE_EXPORT_COMPILE_COMMANDS=ON -B build
# Produces build/compile_commands.json
```

**Makefile / Ninja projects — use `bear`:**
```bash
# Install: apt install bear
bear -- make
bear -- ninja
# Produces compile_commands.json in the current directory
```

**Meson projects:**
```bash
meson setup build
# compile_commands.json is generated automatically in build/
```

---

## Step 2 — Build the index

```bash
/path/to/cxx-index/build/cxxidx index build/compile_commands.json \
    -o /path/to/your/project/index.cxxi \
    -v
```

Options:

| Flag | Description |
|------|-------------|
| `-o <file>` | Output path for the index (default: `index.cxxi`) |
| `-v` | Verbose — print each file as it is processed |
| `--system` | Also index system headers (slower, larger index) |

Indexing time depends on project size. A ~50k-line project takes a few seconds;
large projects (LLVM, Qt) can take several minutes.

---

## Step 3 — Add the project to CodeBrowser

Edit `config.json` in the CodeBrowser directory:

```json
{
    "projects": [
        {
            "name": "myproject",
            "source": "/absolute/path/to/myproject",
            "index":  "index/myproject",
            "cxxindex": "/absolute/path/to/myproject/index.cxxi",
            "compilation-db": "/absolute/path/to/myproject/build/compile_commands.json"
        }
    ]
}
```

| Field | Required | Description |
|-------|----------|-------------|
| `name` | yes | Display name in the UI |
| `source` | yes | Absolute path to the project root |
| `index` | yes | Directory for the full-text search index |
| `cxxindex` | no | Path to the `.cxxi` file — enables graph navigation on startup |
| `compilation-db` | no | Path to `compile_commands.json` — enables **Re-index Now** in Settings |

Restart the server after editing `config.json`.

---

## Loading an index without restarting

You can load (or swap) a `.cxxi` file at runtime without editing `config.json`:

1. Open any file from the project in the editor
2. Click **Settings** in the navbar
3. Enter the `.cxxi` path in the **Index file** field
4. Click **Load**

Statistics (nodes, edges, occurrences) appear immediately and graph features
become available right away.

---

## Re-indexing after code changes

**Via the browser:**
1. Open Settings → click **Re-index Now**
2. A live log appears; the button is disabled while indexing runs

**Via the command line:**
```bash
cxxidx index build/compile_commands.json -o index.cxxi
```

The server picks up the new index on the next graph query — no restart needed
if you used **Load** or if `cxxindex` is set in `config.json`.

---

## Inspecting the index

```bash
# Summary statistics
cxxidx stats index.cxxi

# Find a symbol by name (substring match)
cxxidx find index.cxxi "Parser"

# All callers of a function
cxxidx callers index.cxxi "Parser::parse"

# All callees
cxxidx callees index.cxxi "Parser::parse"

# What symbol is at a given location?
cxxidx at index.cxxi src/parser.cpp 42 10

# Class members
cxxidx members index.cxxi "Parser"

# Inheritance
cxxidx bases   index.cxxi "Dog"
cxxidx derived index.cxxi "Animal"

# All references to a symbol
cxxidx refs index.cxxi "Parser::parse"
```

---

## Troubleshooting

**"No symbol at this position" in the graph panel**
The cursor is on whitespace, a keyword, or a type qualifier rather than on a
symbol name. Place the cursor directly on the function or class name token.

**Graph shows "(none)" for callers/callees**
The symbol exists in the index but has no recorded call edges. This happens when:
- The callers are in files that were not in `compile_commands.json`
- The symbol is an interface method called through a virtual dispatch the
  indexer could not resolve statically

**Index file not found after startup**
The `cxxindex` path in `config.json` must be an absolute path. Relative paths
are resolved from the CodeBrowser working directory at startup, which may differ
from where you expect.

**System headers not indexed (e.g. STL symbols have no definition)**
Re-run with `--system`:
```bash
cxxidx index build/compile_commands.json -o index.cxxi --system
```
Note: this significantly increases index size and indexing time.
