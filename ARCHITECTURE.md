# CodeBrowser — Architecture

Web-based code browser inspired by Sourcetrail and OpenGrok, running in the browser.
Monaco Editor displays code; all navigation and graph analysis is handled by the
C++ Crow server backed by a configurable symbol provider.

## What already exists

- **C++ Crow HTTP server** — 14+ routes, Lua templating, MIME handling
- **Monaco editor** — syntax highlighting, context menu (definition, declaration,
  references, implementation, typeDefinition, typeHierarchy, switchHeader)
- **LspClient** — generic LSP client (clangd), via PipeAdapter (stdio pipe)
- **CtagsProvider** — ctags-based symbol navigation, hover, grep references
- **Git integration** — blame/annotate, file/folder history, diff view
- **Full-text search** — Apache Lucy (optional) or grep/ripgrep fallback
- **Multi-project** — multiple source trees via config.json

## What we are adding (Phase A → Phase B)

```
Phase A — cxxlsp + cxxidx backend
  ├── cxxlsp as LSP symbol provider (definition, hover, references, declaration)
  ├── CxxIndexProvider wrapping cxxidx CLI (callers, callees, bases, derived, members)
  └── New routes: /callers /callees /bases /derived /members /index-status

Phase B — Graph visualization (Sourcetrail parity)
  ├── Cytoscape.js panel in editor.html
  ├── Context menu: "Show Callers", "Show Callees", "Show Hierarchy", "Show Members"
  └── Clickable graph nodes → navigate Monaco to definition

Phase C — Backend selector UI + index management
  ├── /backends route — detect available backends + versions
  ├── Settings modal in editor.html
  └── Re-index button with SSE progress stream
```

## System Overview

```
Browser
  Monaco (read-only)          Graph Panel (Cytoscape)      Symbol Panel
  click → file:line:col   →   /callers, /callees           /identifier refs
        ↓                     /bases, /derived              hover overlay
  REST API (Crow, port 3000)
        ↓
  Symbol provider (config: symbol-provider)
    "ctags"  → CtagsProvider (ctags + grep)
    "clangd" → LspClient    (clangd via PipeAdapter)
    "cxxlsp" → LspClient    (cxxlsp via PipeAdapter)  ← NEW
             + CxxIndexProvider (cxxidx CLI)          ← NEW
```

## New Config Fields

```json
{
  "symbol-provider": "cxxlsp",
  "lsp": "/path/to/cxxlsp",
  "cxxidx": "/path/to/cxxidx",
  "projects": [
    {
      "name": "myproject",
      "source": "/src/myproject",
      "index":  "index/myproject",
      "cxxindex": "/src/myproject/index.cxxi"
    }
  ]
}
```

## New REST API Routes (Phase A)

```
GET /callers?project=&file=&line=&col=
GET /callees?project=&file=&line=&col=
GET /bases?project=&file=&line=&col=
GET /derived?project=&file=&line=&col=
GET /members?project=&file=&line=&col=
GET /index-status?project=
```

### Graph JSON response shape

```json
{
  "center": {"name": "Parser::parse", "kind": "method"},
  "nodes": [
    {"id": 1, "name": "Parser::parse", "kind": "method", "file": "/src/p.cpp", "line": 42},
    {"id": 2, "name": "main",          "kind": "function", "file": "/src/main.cpp", "line": 10}
  ],
  "edges": [
    {"from": 2, "to": 1}
  ]
}
```

### Index status response

```json
{
  "files": 42,
  "nodes": 2431,
  "edges": 891,
  "occurrences": 15432,
  "indexPath": "/src/myproject/index.cxxi",
  "available": true
}
```

## File Map

```
work/
  Config.h / Config.cpp          ← add cxxidx, per-project cxxindex
  lsp/PipeAdapter.cpp            ← use sh -c for args support
  cxxindex/
    CxxIndexProvider.h           ← NEW
    CxxIndexProvider.cpp         ← NEW
  Server.h / Server.cpp          ← add CxxIndexProvider, new routes
  CMakeLists.txt                 ← add CxxIndexProvider.cpp

root/
  files/
    cytoscape.min.js             ← NEW (Phase B)
    graph.js                     ← NEW (Phase B)
  editor.html                    ← add graph panel (Phase B)
  files/app.js                   ← add graph context menu (Phase B)
```

## Comparison with Sourcetrail

| Sourcetrail (Qt)              | CodeBrowser (browser)              |
|-------------------------------|------------------------------------|
| QGraphicsView custom graphs   | Cytoscape.js + dagre layout        |
| Integrated C++ indexer        | cxx-index (cxxidx + cxxlsp)       |
| C/C++ only                    | Multi-backend (ctags = any lang)   |
| Desktop app                   | Browser (mobile via claude-mobile) |
| No LSP                        | cxxlsp / clangd / lsp_ctags        |
