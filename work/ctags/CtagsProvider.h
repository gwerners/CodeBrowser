#pragma once
#include <map>
#include <string>
#include <vector>
#include "nlohmann/json.hpp"

using json = nlohmann::json;

// A single symbol entry parsed from ctags output.
struct CtagsEntry {
  std::string name;
  std::string file;
  int line = 0;
  std::string kind;       // f=function, c=class, s=struct, m=member, etc.
  std::string signature;  // function signature if available
  std::string scope;      // class/struct scope if available
};

// CtagsProvider: alternative to clangd for symbol navigation.
// Parses Universal Ctags output to provide definition lookups.
// Does NOT require compile_commands.json.
//
// Usage:
//   CtagsProvider provider;
//   provider.index("/path/to/source");  // runs ctags
//   provider.loadTags("/path/to/source/tags");  // or load existing
//   auto result = provider.findDefinition("myFunction");
class CtagsProvider {
 public:
  CtagsProvider();

  // Run ctags on a source directory and load results
  bool index(const std::string& sourceDir);

  // Load an existing tags file
  bool loadTags(const std::string& tagsFile);

  // Find definition(s) of a symbol
  std::vector<CtagsEntry> findDefinition(const std::string& symbol) const;

  // Find all symbols in a file
  std::vector<CtagsEntry> findSymbolsInFile(const std::string& file) const;

  // Find symbol at a specific file and line (approximate)
  CtagsEntry findSymbolAtLine(const std::string& file, int line) const;

  // Get hover info for a symbol (kind + signature)
  std::string getHoverInfo(const std::string& symbol) const;

  // Convert result to JSON matching the LSP-like response format
  // used by the /identifier and /hover routes
  json definitionToJson(const std::string& symbol,
                        const std::string& projectRoot,
                        const std::string& projectName,
                        const std::string& serverUrl) const;

  // Type definition: filter to class/struct/enum/typedef only
  json typeDefinitionToJson(const std::string& symbol,
                            const std::string& projectRoot,
                            const std::string& projectName,
                            const std::string& serverUrl) const;

  // Implementation: filter to function/method definitions only
  json implementationToJson(const std::string& symbol,
                            const std::string& projectRoot,
                            const std::string& projectName,
                            const std::string& serverUrl) const;

  json hoverToJson(const std::string& symbol) const;

  // Find references: grep for symbol across all indexed source files
  json referencesToJson(const std::string& symbol,
                        const std::string& projectRoot,
                        const std::string& projectName,
                        const std::string& serverUrl) const;

  void setSourceDir(const std::string& dir) { _sourceDir = dir; }
  bool isLoaded() const { return _loaded; }
  size_t symbolCount() const { return _symbols.size(); }
  const std::string& sourceDir() const { return _sourceDir; }

 private:
  bool parseLine(const std::string& line);

  // symbol name -> list of entries (can have multiple definitions)
  std::multimap<std::string, CtagsEntry> _symbols;
  // file -> list of entries
  std::multimap<std::string, CtagsEntry> _fileIndex;
  bool _loaded = false;
  std::string _sourceDir;
};
