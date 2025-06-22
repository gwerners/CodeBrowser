#pragma once
#include "PipeAdapter.h"
#include "fmt/color.h"
#include "nlohmann/json.hpp"

using json = nlohmann::json;
#include "iostream"

const json hover_default =
    {}; /*{{"id", 1},
      {"jsonrpc", "2.0"},
      {"description", "No Info\n\nmissing information"}};*/

class Initialize {
 public:
  Initialize(const std::string& lsp)
      : _clangd(PipeAdapter::getInstance(lsp)) {}  //"--pretty" for clangd
  void send() {
    json initialize = {
        {"jsonrpc", "2.0"},
        {"id", 1},
        {"method", "initialize"},
        {"params",
         {{"processId", 1},
          {"rootUri", "/home/gwerners/development/CodeWayfinder/dhewm3"},
          {"capabilities", {{"workspace", {{"applyEdit", false}}}}}}}};
    // params is  InitializeParams
    std::cout << initialize.dump(4) << std::endl;
    _clangd->send(initialize.dump());
  }
  std::string receive() { return _clangd->receive(); }

 private:
  PipeAdapter* _clangd;
  std::string _lastFile;
};

class DidClose {
 public:
  DidClose(const std::string& lsp, const std::string& lastFile)
      : _clangd(PipeAdapter::getInstance(lsp)), _lastFile(lastFile) {}
  void send() {
    json didClose = {
        {"jsonrpc", "2.0"},
        {"method", "textDocument/didClose"},
        {"params",
         {{"textDocument", {{"uri", "file://" + _lastFile}, {"version", 1}}}}}};
    _clangd->send(didClose.dump());
  }
  std::string receive() { return _clangd->receive(); }

 private:
  PipeAdapter* _clangd;
  std::string _lastFile;
};

class DidOpen {
 public:
  DidOpen(const std::string& lsp,
          const std::string& root,
          const std::string& content)
      : _clangd(PipeAdapter::getInstance(lsp)),
        _root(root),
        _content(content) {}
  void send() {
    json didOpen = {{"jsonrpc", "2.0"},
                    {"method", "textDocument/didOpen"},
                    {"params",
                     {{"textDocument",
                       {{"uri", "file://" + _root},
                        {"languageId", "cpp"},
                        {"version", 1},
                        {"text", _content}}}}}};
    _clangd->send(didOpen.dump());
  }
  std::string receive() { return _clangd->receive(); }

 private:
  PipeAdapter* _clangd;
  std::string _root;
  std::string _content;
};

class Hover {
 public:
  Hover(const std::string& lsp,
        const std::string& lastFile,
        int line,
        int column)
      : _clangd(PipeAdapter::getInstance(lsp)),
        _lastFile(lastFile),
        _line(line),
        _column(column) {}
  void send() {
    json hover = {
        {"jsonrpc", "2.0"},
        {"id", 1},
        {"method", "textDocument/hover"},
        {"params",
         {{"textDocument", {{"uri", "file://" + _lastFile}}},
          {"position", {{"line", _line - 1}, {"character", _column - 1}}}}}};
    fmt::print(fmt::emphasis::bold | fg(fmt::color::blue),
               "hover msg to clangd [{}]\n", hover.dump(4));
    _clangd->send(hover.dump());
  }
  std::string receive() { return _clangd->receive(); }

 private:
  PipeAdapter* _clangd;
  std::string _lastFile;
  int _line;
  int _column;
};

class HoverAnswer {
 public:
  static json out(const json& j) {
    if (j.empty()) {
      return hover_default;
    }
    if (j.contains("method")) {
      std::string methodString = j["method"];
      if (methodString.compare("textDocument/publishDiagnostics") == 0) {
        return hover_default;
      }
    }
    json result;
    json contents;
    if (j.contains("result")) {
      result = j["result"];
      if (result.is_null()) {
        return hover_default;
      }
    }
    if (result.contains("contents")) {
      contents = result["contents"];
      if (contents.is_null() || !contents.contains("value")) {
        return hover_default;
      }
    }
    std::string strContents = contents["value"];
    if (strContents.compare(0, 7, "No Info") == 0) {
      return hover_default;
    }
    json answer = {{"id", 1}, {"jsonrpc", "2.0"}, {"description", strContents}};
    return answer;
  }
};

class Definition {
 public:
  Definition(const std::string& lsp,
             const std::string& lastFile,
             const std::string& identifier,
             int line,
             int column)
      : _clangd(PipeAdapter::getInstance(lsp)),
        _lastFile(lastFile),
        _identifier(identifier),
        _line(line),
        _column(column) {}
  void send() {
    json definition = {
        {"jsonrpc", "2.0"},
        {"id", 1},
        {"method", "textDocument/definition"},
        {"params",
         {{"textDocument", {{"uri", "file://" + _lastFile}}},
          {"position", {{"line", _line - 1}, {"character", _column - 1}}}}}};
    fmt::print(fmt::emphasis::bold | fg(fmt::color::blue),
               "definition msg to clangd [{}]\n", definition.dump(4));
    _clangd->send(definition.dump());
  }
  std::string receive() { return _clangd->receive(); }

 private:
  PipeAdapter* _clangd;
  std::string _lastFile;
  std::string _identifier;
  int _line;
  int _column;
};

class DefinitionAnswer {
 public:
  static json out(const json& j) {
    json _default = {};
    std::string uri;
    fmt::print(fmt::emphasis::bold | fg(fmt::color::blue), "[{}]\n", j.dump(4));
    if (j.empty()) {
      return _default;
    }
    if (j.contains("uri")) {
      uri = j["uri"];
    } else if (j.contains("targetUri")) {
      uri = j["targetUri"];
    }
    json answer = {{"id", 1}, {"jsonrpc", "2.0"}, {"uri", uri}};
    return answer;
  }
};

class Declaration {
 public:
  Declaration(const std::string& lsp,
              const std::string& lastFile,
              const std::string& identifier,
              int line,
              int column)
      : _clangd(PipeAdapter::getInstance(lsp)),
        _lastFile(lastFile),
        _identifier(identifier),
        _line(line),
        _column(column) {}
  void send() {
    json declaration = {
        {"jsonrpc", "2.0"},
        {"id", 1},
        {"method", "textDocument/declaration"},
        {"params",
         {{"textDocument", {{"uri", "file://" + _lastFile}}},
          {"position", {{"line", _line - 1}, {"character", _column - 1}}}}}};
    fmt::print(fmt::emphasis::bold | fg(fmt::color::blue),
               "declaration msg to clangd [{}]\n", declaration.dump(4));
    _clangd->send(declaration.dump());
  }
  std::string receive() { return _clangd->receive(); }

 private:
  PipeAdapter* _clangd;
  std::string _lastFile;
  std::string _identifier;
  int _line;
  int _column;
};

class DeclarationAnswer {
 public:
  static json out(const json& j) {
    json _default = {};
    std::string uri;
    fmt::print(fmt::emphasis::bold | fg(fmt::color::blue), "[{}]\n", j.dump(4));
    if (j.empty()) {
      return _default;
    }
    if (j.contains("uri")) {
      uri = j["uri"];
    } else if (j.contains("targetUri")) {
      uri = j["targetUri"];
    }
    json answer = {{"id", 1}, {"jsonrpc", "2.0"}, {"uri", uri}};
    return answer;
  }
};

class References {
 public:
  References(const std::string& lsp,
             const std::string& lastFile,
             const std::string& identifier,
             int line,
             int column)
      : _clangd(PipeAdapter::getInstance(lsp)),
        _lastFile(lastFile),
        _identifier(identifier),
        _line(line),
        _column(column) {}
  void send() {
    json references = {
        {"jsonrpc", "2.0"},
        {"id", 1},
        {"method", "textDocument/references"},
        {"params",
         {{"textDocument", {{"uri", "file://" + _lastFile}}},
          {"position", {{"line", _line - 1}, {"character", _column - 1}}}}}};
    fmt::print(fmt::emphasis::bold | fg(fmt::color::blue),
               "declaration msg to clangd [{}]\n", references.dump(4));
    _clangd->send(references.dump());
  }
  std::string receive() { return _clangd->receive(); }

 private:
  PipeAdapter* _clangd;
  std::string _lastFile;
  std::string _identifier;
  int _line;
  int _column;
};

class ReferencesAnswer {
 public:
  static json out(const json& j) {
    json _default = {};
    std::string uri;
    fmt::print(fmt::emphasis::bold | fg(fmt::color::blue), "[{}]\n", j.dump(4));
    if (j.empty()) {
      return _default;
    }
    if (j.contains("uri")) {
      uri = j["uri"];
    } else if (j.contains("targetUri")) {
      uri = j["targetUri"];
    }
    json answer = {{"id", 1}, {"jsonrpc", "2.0"}, {"uri", uri}};
    return answer;
  }
};
// https://github.com/microsoft/vscode-languageserver-node/pull/426/files#diff-927616f8461039b729d40b4dedc61a033f5ba2ee8321f04e24e138711ff3bb9b
/*
 TypeHierarchyDirection

  Flag for retrieving/resolving the subtypes.
  export const Children = 0;

  Flag to use when retrieving/resolving the supertypes.
  export const Parents = 1;

  Flag for resolving both the super- and subtypes.
  export const Both = 2;


export namespace SymbolKind {
    export const File = 1;
    export const Module = 2;
    export const Namespace = 3;
    export const Package = 4;
    export const Class = 5;
    export const Method = 6;
    export const Property = 7;
    export const Field = 8;
    export const Constructor = 9;
    export const Enum = 10;
    export const Interface = 11;
    export const Function = 12;
    export const Variable = 13;
    export const Constant = 14;
    export const String = 15;
    export const Number = 16;
    export const Boolean = 17;
    export const Array = 18;
    export const Object = 19;
    export const Key = 20;
    export const Null = 21;
    export const EnumMember = 22;
    export const Struct = 23;
    export const Event = 24;
    export const Operator = 25;
    export const TypeParameter = 26;
}
*/
class TypeHierarchy {
 public:
  TypeHierarchy(const std::string& lsp,
                const std::string& lastFile,
                const std::string& identifier,
                int line,
                int column)
      : _clangd(PipeAdapter::getInstance(lsp)),
        _lastFile(lastFile),
        _identifier(identifier),
        _line(line),
        _column(column) {}
  void send() {
    json typeHierarchy = {
        {"jsonrpc", "2.0"},
        {"id", 1},
        {"method", "textDocument/typeHierarchy"},
        {"params",
         {{"textDocument", {{"uri", "file://" + _lastFile}}},
          {"resolve", 999},
          {"direction", 2},
          {"position", {{"line", _line - 1}, {"character", _column - 1}}}}}};

    fmt::print(fmt::emphasis::bold | fg(fmt::color::blue),
               "declaration msg to clangd [{}]\n", typeHierarchy.dump(4));
    _clangd->send(typeHierarchy.dump());
  }
  std::string receive() { return _clangd->receive(); }

 private:
  PipeAdapter* _clangd;
  std::string _lastFile;
  std::string _identifier;
  int _line;
  int _column;
};

class TypeHierarchyAnswer {
 public:
  static json out(const json& j) {
    json _default = {};
    std::string result;
    fmt::print(fmt::emphasis::bold | fg(fmt::color::blue), "[{}]\n", j.dump(4));
    if (j.empty()) {
      return _default;
    }
    if (j.contains("result")) {
      result = j["result"].dump();
    }
    json answer = {{"id", 1}, {"jsonrpc", "2.0"}, {"result", result}};
    return answer;
  }
};

class HeaderSource {
 public:
  HeaderSource(const std::string& lsp, const std::string& lastFile)
      : _clangd(PipeAdapter::getInstance(lsp)), _lastFile(lastFile) {}
  void send() {
    json header_source = {{"jsonrpc", "2.0"},
                          {"id", 1},
                          {"method", "textDocument/switchSourceHeader"},
                          {"params", {{"uri", "file://" + _lastFile}}}};
    fmt::print(fmt::emphasis::bold | fg(fmt::color::blue),
               "declaration msg to clangd [{}]\n", header_source.dump(4));
    _clangd->send(header_source.dump());
  }
  std::string receive() { return _clangd->receive(); }

 private:
  PipeAdapter* _clangd;
  std::string _lastFile;
};

class HeaderSourceAnswer {
 public:
  static json out(const json& j) {
    json _default = {};
    std::string uri;
    fmt::print(fmt::emphasis::bold | fg(fmt::color::blue), "[{}]\n", j.dump(4));
    if (j.empty()) {
      return _default;
    }
    if (j.contains("uri")) {
      uri = j["uri"];
    } else if (j.contains("targetUri")) {
      uri = j["targetUri"];
    }
    json answer = {{"id", 1}, {"jsonrpc", "2.0"}, {"uri", uri}};
    return answer;
  }
};

class TypeDefinition {
 public:
  TypeDefinition(const std::string& lsp,
                 const std::string& lastFile,
                 const std::string& identifier,
                 int line,
                 int column)
      : _clangd(PipeAdapter::getInstance(lsp)),
        _lastFile(lastFile),
        _identifier(identifier),
        _line(line),
        _column(column) {}
  void send() {
    json typeDefinition = {
        {"jsonrpc", "2.0"},
        {"id", 1},
        {"method", "textDocument/typeDefinition"},
        {"params",
         {{"textDocument", {{"uri", "file://" + _lastFile}}},
          {"position", {{"line", _line - 1}, {"character", _column - 1}}}}}};
    fmt::print(fmt::emphasis::bold | fg(fmt::color::blue),
               "typeDefinition msg to clangd [{}]\n", typeDefinition.dump(4));
    _clangd->send(typeDefinition.dump());
  }
  std::string receive() { return _clangd->receive(); }

 private:
  PipeAdapter* _clangd;
  std::string _lastFile;
  std::string _identifier;
  int _line;
  int _column;
};

class TypeDefinitionAnswer {
 public:
  static json out(const json& j) {
    json _default = {};
    std::string uri;
    fmt::print(fmt::emphasis::bold | fg(fmt::color::blue), "[{}]\n", j.dump(4));
    if (j.empty()) {
      return _default;
    }
    if (j.contains("uri")) {
      uri = j["uri"];
    } else if (j.contains("targetUri")) {
      uri = j["targetUri"];
    }
    json answer = {{"id", 1}, {"jsonrpc", "2.0"}, {"uri", uri}};
    return answer;
  }
};

class Implementation {
 public:
  Implementation(const std::string& lsp,
                 const std::string& lastFile,
                 const std::string& identifier,
                 int line,
                 int column)
      : _clangd(PipeAdapter::getInstance(lsp)),
        _lastFile(lastFile),
        _identifier(identifier),
        _line(line),
        _column(column) {}
  void send() {
    json implementation = {
        {"jsonrpc", "2.0"},
        {"id", 1},
        {"method", "textDocument/implementation"},
        {"params",
         {{"textDocument", {{"uri", "file://" + _lastFile}}},
          {"position", {{"line", _line - 1}, {"character", _column - 1}}}}}};
    fmt::print(fmt::emphasis::bold | fg(fmt::color::blue),
               "implementation msg to clangd [{}]\n", implementation.dump(4));
    _clangd->send(implementation.dump());
  }
  std::string receive() { return _clangd->receive(); }

 private:
  PipeAdapter* _clangd;
  std::string _lastFile;
  std::string _identifier;
  int _line;
  int _column;
};

class ImplementationAnswer {
 public:
  static json out(const json& j) {
    json _default = {};
    std::string uri;
    fmt::print(fmt::emphasis::bold | fg(fmt::color::blue), "[{}]\n", j.dump(4));
    if (j.empty()) {
      return _default;
    }
    if (j.contains("uri")) {
      uri = j["uri"];
    } else if (j.contains("targetUri")) {
      uri = j["targetUri"];
    }
    json answer = {{"id", 1}, {"jsonrpc", "2.0"}, {"uri", uri}};
    return answer;
  }
};
