#pragma once
#include <string>
#include "nlohmann/json.hpp"

using json = nlohmann::json;

// CxxIndexProvider: wraps the cxxidx CLI tool to provide graph-level queries
// on a pre-built .cxxi index (call graph, inheritance, member relationships).
//
// All graph-returning methods produce the same shape:
//   { "center": {name, kind},
//     "nodes":  [{id, name, kind, file, line},...],
//     "edges":  [{from, to},...] }
//
// Requires cxxidx to be in PATH or configured via Config::cxxidx.
class CxxIndexProvider {
 public:
  CxxIndexProvider(const std::string& cxxidxBin, const std::string& indexPath);

  // Symbol at file:line:col — used to resolve cursor position to symbol name
  // Returns {name, kind, defFile, defLine, memberCount, callerCount, calleeCount}
  // or {} if not found.
  json symbolAt(const std::string& file, int line, int col) const;

  // Graph queries — all resolve cursor position to symbol first
  json callers(const std::string& file, int line, int col) const;
  json callees(const std::string& file, int line, int col) const;
  json bases(const std::string& file, int line, int col) const;
  json derived(const std::string& file, int line, int col) const;
  json members(const std::string& file, int line, int col) const;

  // Graph query by symbol name directly
  json callersByName(const std::string& symbol) const;
  json calleesByName(const std::string& symbol) const;
  json basesByName(const std::string& symbol) const;
  json derivedByName(const std::string& symbol) const;
  json membersByName(const std::string& symbol) const;

  // Index statistics
  json status() const;

  bool isAvailable() const;
  const std::string& indexPath() const { return _indexPath; }

 private:
  // Run cxxidx with given args, return stdout
  std::string run(const std::string& subcmd, const std::string& arg1 = "",
                  const std::string& arg2 = "") const;

  // Parse cxxidx text output into structured JSON
  json parseCallGraph(const std::string& output,
                      const std::string& centerName,
                      const std::string& centerKind,
                      bool edgeToCenter) const;
  json parseInheritanceGraph(const std::string& output,
                              const std::string& centerName,
                              const std::string& centerKind,
                              bool edgeToCenter) const;
  json parseMembersGraph(const std::string& output,
                          const std::string& centerName,
                          const std::string& centerKind) const;
  json parseSymbolAt(const std::string& output) const;
  json parseStats(const std::string& output) const;

 public:
  // Parse "path:line:col" string — public so free helpers in .cpp can use it
  static bool parseLocation(const std::string& loc,
                             std::string& file, int& line, int& col);

  std::string _cxxidxBin;
  std::string _indexPath;
};
