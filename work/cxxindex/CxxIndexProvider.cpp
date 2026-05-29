#include "cxxindex/CxxIndexProvider.h"
#include "PipeCommand.h"
#include "fmt/color.h"
#include "fmt/format.h"
#include <filesystem>
#include <sstream>

namespace fs = std::filesystem;

// ---------------------------------------------------------------------------
// Construction
// ---------------------------------------------------------------------------

CxxIndexProvider::CxxIndexProvider(const std::string& cxxidxBin,
                                   const std::string& indexPath)
    : _cxxidxBin(cxxidxBin), _indexPath(indexPath) {}

bool CxxIndexProvider::isAvailable() const {
  return !_indexPath.empty() && fs::exists(_indexPath);
}

// ---------------------------------------------------------------------------
// Run cxxidx subcommand
// ---------------------------------------------------------------------------

std::string CxxIndexProvider::run(const std::string& subcmd,
                                  const std::string& arg1,
                                  const std::string& arg2) const {
  if (!isAvailable()) return "";

  if (arg2.empty() && arg1.empty())
    return PipeCommand::cmd(_cxxidxBin, subcmd.c_str(), _indexPath.c_str());

  if (arg2.empty())
    return PipeCommand::cmd(_cxxidxBin, subcmd.c_str(), _indexPath.c_str(),
                            arg1.c_str());

  return PipeCommand::cmd(_cxxidxBin, subcmd.c_str(), _indexPath.c_str(),
                          arg1.c_str(), arg2.c_str());
}

// ---------------------------------------------------------------------------
// Parse helpers
// ---------------------------------------------------------------------------

// Parse "path:line:col" from cxxidx locStr output.
bool CxxIndexProvider::parseLocation(const std::string& loc,
                                     std::string& file, int& line, int& col) {
  // Find last two colons
  auto p2 = loc.rfind(':');
  if (p2 == std::string::npos) return false;
  auto p1 = loc.rfind(':', p2 - 1);
  if (p1 == std::string::npos) return false;

  try {
    col  = std::stoi(loc.substr(p2 + 1));
    line = std::stoi(loc.substr(p1 + 1, p2 - p1 - 1));
    file = loc.substr(0, p1);
    return true;
  } catch (...) {
    return false;
  }
}

// Parse a line like "  function        Parser::parse  (path:line:col)"
// Returns {name, kind, file, line} or empty on failure.
static json parseEntryLine(const std::string& rawLine) {
  std::string line = rawLine;
  // Strip leading whitespace
  size_t start = line.find_first_not_of(" \t");
  if (start == std::string::npos) return {};
  line = line.substr(start);

  // Extract location "(path:line:col)" at end
  std::string file;
  int fileLine = 0, fileCol = 0;
  bool hasLoc = false;
  auto lp = line.rfind('(');
  auto rp = line.rfind(')');
  if (lp != std::string::npos && rp != std::string::npos && rp > lp) {
    std::string locStr = line.substr(lp + 1, rp - lp - 1);
    hasLoc = CxxIndexProvider::parseLocation(locStr, file, fileLine, fileCol);
    line = line.substr(0, lp);
  }

  // Strip trailing whitespace
  auto end = line.find_last_not_of(" \t");
  if (end != std::string::npos) line = line.substr(0, end + 1);

  // Split on first run of 2+ spaces: "kind  name"
  auto sep = line.find("  ");
  if (sep == std::string::npos) return {};
  std::string kind = line.substr(0, sep);
  std::string name = line.substr(line.find_first_not_of(" ", sep));

  json node;
  node["name"] = name;
  node["kind"] = kind;
  if (hasLoc) {
    node["file"] = file;
    node["line"] = fileLine;
  }
  return node;
}

// ---------------------------------------------------------------------------
// symbolAt — cxxidx at <index> <file> <line> <col>
// ---------------------------------------------------------------------------

json CxxIndexProvider::parseSymbolAt(const std::string& output) const {
  if (output.empty() || output.find("No symbol at") != std::string::npos)
    return {};

  std::istringstream ss(output);
  std::string line;
  json result;
  bool firstLine = true;

  while (std::getline(ss, line)) {
    if (firstLine) {
      // "kind  fully::qualified::name"
      auto sep = line.find("  ");
      if (sep != std::string::npos) {
        result["kind"] = line.substr(0, sep);
        result["name"] = line.substr(line.find_first_not_of(" ", sep));
      }
      firstLine = false;
      continue;
    }
    if (line.rfind("Defined at: ", 0) == 0) {
      std::string loc = line.substr(12);
      std::string file; int l = 0, c = 0;
      if (parseLocation(loc, file, l, c)) {
        result["defFile"] = file;
        result["defLine"] = l;
        result["defCol"]  = c;
      }
    } else if (line.rfind("Members:", 0) == 0) {
      try { result["memberCount"] = std::stoi(line.substr(8)); } catch (...) {}
    } else if (line.rfind("Callers:", 0) == 0) {
      try { result["callerCount"] = std::stoi(line.substr(8)); } catch (...) {}
    } else if (line.rfind("Callees:", 0) == 0) {
      try { result["calleeCount"] = std::stoi(line.substr(8)); } catch (...) {}
    } else if (line.rfind("References:", 0) == 0) {
      try { result["referenceCount"] = std::stoi(line.substr(11)); } catch (...) {}
    }
  }
  return result;
}

json CxxIndexProvider::symbolAt(const std::string& file, int line, int col) const {
  std::string out = run("at", file,
                        std::to_string(line) + " " + std::to_string(col));
  // cxxidx at takes 4 args; PipeCommand doesn't support that directly — use
  // the 2-arg form with a composed arg string is not possible. Use run directly.
  // Actually we need 4 args: cxxidx at <index> <file> <line> <col>
  std::string result = PipeCommand::cmd(
      _cxxidxBin, "at", _indexPath.c_str(), file.c_str(),
      std::to_string(line).c_str(), std::to_string(col).c_str());
  return parseSymbolAt(result);
}

// ---------------------------------------------------------------------------
// Call graph — cxxidx callers/callees <index> <symbol>
// ---------------------------------------------------------------------------

json CxxIndexProvider::parseCallGraph(const std::string& output,
                                       const std::string& centerName,
                                       const std::string& centerKind,
                                       bool edgeToCenter) const {
  json result;
  result["center"] = {{"name", centerName}, {"kind", centerKind}};
  result["nodes"]  = json::array();
  result["edges"]  = json::array();

  if (output.empty()) return result;

  // Center node gets id=0
  json center;
  center["id"]   = 0;
  center["name"] = centerName;
  center["kind"] = centerKind;
  result["nodes"].push_back(center);

  std::istringstream ss(output);
  std::string line;
  int nextId = 1;

  while (std::getline(ss, line)) {
    // Skip header line ("Callers of: ..." or "Callees of: ...")
    if (line.find("Callers of:") != std::string::npos) continue;
    if (line.find("Callees of:") != std::string::npos) continue;
    if (line == "  (none)") continue;
    if (line.empty()) continue;

    auto node = parseEntryLine(line);
    if (node.empty()) continue;

    node["id"] = nextId;
    result["nodes"].push_back(node);

    json edge;
    if (edgeToCenter) {
      edge["from"] = nextId;
      edge["to"]   = 0;
    } else {
      edge["from"] = 0;
      edge["to"]   = nextId;
    }
    result["edges"].push_back(edge);
    nextId++;
  }
  return result;
}

json CxxIndexProvider::callersByName(const std::string& symbol) const {
  auto sym = parseSymbolAt(run("at", symbol));
  std::string kind = sym.value("kind", "symbol");
  return parseCallGraph(run("callers", symbol), symbol, kind, true);
}

json CxxIndexProvider::calleesByName(const std::string& symbol) const {
  auto sym = parseSymbolAt(run("at", symbol));
  std::string kind = sym.value("kind", "symbol");
  return parseCallGraph(run("callees", symbol), symbol, kind, false);
}

json CxxIndexProvider::callers(const std::string& file, int line, int col) const {
  auto sym = symbolAt(file, line, col);
  if (sym.empty()) return {};
  std::string name = sym.value("name", "");
  std::string kind = sym.value("kind", "symbol");
  if (name.empty()) return {};
  return parseCallGraph(run("callers", name), name, kind, true);
}

json CxxIndexProvider::callees(const std::string& file, int line, int col) const {
  auto sym = symbolAt(file, line, col);
  if (sym.empty()) return {};
  std::string name = sym.value("name", "");
  std::string kind = sym.value("kind", "symbol");
  if (name.empty()) return {};
  return parseCallGraph(run("callees", name), name, kind, false);
}

// ---------------------------------------------------------------------------
// Inheritance graph — cxxidx bases/derived <index> <symbol>
// ---------------------------------------------------------------------------

json CxxIndexProvider::parseInheritanceGraph(const std::string& output,
                                              const std::string& centerName,
                                              const std::string& centerKind,
                                              bool edgeToCenter) const {
  json result;
  result["center"] = {{"name", centerName}, {"kind", centerKind}};
  result["nodes"]  = json::array();
  result["edges"]  = json::array();

  json center;
  center["id"]   = 0;
  center["name"] = centerName;
  center["kind"] = centerKind;
  result["nodes"].push_back(center);

  std::istringstream ss(output);
  std::string line;
  int nextId = 1;

  while (std::getline(ss, line)) {
    if (line.find("Base classes of:") != std::string::npos)  continue;
    if (line.find("Classes derived from:") != std::string::npos) continue;
    if (line == "  (none)") continue;
    if (line.empty()) continue;

    auto node = parseEntryLine(line);
    if (node.empty()) continue;

    node["id"] = nextId;
    result["nodes"].push_back(node);

    json edge;
    if (edgeToCenter) { edge["from"] = nextId; edge["to"] = 0; }
    else              { edge["from"] = 0;       edge["to"] = nextId; }
    result["edges"].push_back(edge);
    nextId++;
  }
  return result;
}

json CxxIndexProvider::basesByName(const std::string& symbol) const {
  auto sym = parseSymbolAt(run("at", symbol));
  std::string kind = sym.value("kind", "class");
  return parseInheritanceGraph(run("bases", symbol), symbol, kind, false);
}

json CxxIndexProvider::derivedByName(const std::string& symbol) const {
  auto sym = parseSymbolAt(run("at", symbol));
  std::string kind = sym.value("kind", "class");
  return parseInheritanceGraph(run("derived", symbol), symbol, kind, true);
}

json CxxIndexProvider::bases(const std::string& file, int line, int col) const {
  auto sym = symbolAt(file, line, col);
  if (sym.empty()) return {};
  std::string name = sym.value("name", "");
  std::string kind = sym.value("kind", "class");
  if (name.empty()) return {};
  return parseInheritanceGraph(run("bases", name), name, kind, false);
}

json CxxIndexProvider::derived(const std::string& file, int line, int col) const {
  auto sym = symbolAt(file, line, col);
  if (sym.empty()) return {};
  std::string name = sym.value("name", "");
  std::string kind = sym.value("kind", "class");
  if (name.empty()) return {};
  return parseInheritanceGraph(run("derived", name), name, kind, true);
}

// ---------------------------------------------------------------------------
// Members graph — cxxidx members <index> <symbol>
// ---------------------------------------------------------------------------

json CxxIndexProvider::parseMembersGraph(const std::string& output,
                                          const std::string& centerName,
                                          const std::string& centerKind) const {
  json result;
  result["center"] = {{"name", centerName}, {"kind", centerKind}};
  result["nodes"]  = json::array();
  result["edges"]  = json::array();

  json center;
  center["id"]   = 0;
  center["name"] = centerName;
  center["kind"] = centerKind;
  result["nodes"].push_back(center);

  std::istringstream ss(output);
  std::string line;
  std::string currentKind;
  int nextId = 1;

  while (std::getline(ss, line)) {
    // Header: "class  ClassName" or "struct  StructName" — skip
    if (line.rfind("Members:", 0) == 0) continue;
    if (line.find("  [") != std::string::npos && line.find("]") != std::string::npos) {
      // Kind group header "[method]", "[field]", etc.
      auto lb = line.find('['), rb = line.find(']');
      if (lb != std::string::npos && rb != std::string::npos)
        currentKind = line.substr(lb + 1, rb - lb - 1);
      continue;
    }
    // Member name line: "    FullyQualified::name"
    size_t start = line.find_first_not_of(" \t");
    if (start == std::string::npos || line.empty()) continue;
    std::string name = line.substr(start);
    if (name.empty()) continue;

    json node;
    node["id"]   = nextId;
    node["name"] = name;
    node["kind"] = currentKind;
    result["nodes"].push_back(node);

    json edge;
    edge["from"] = 0;
    edge["to"]   = nextId;
    result["edges"].push_back(edge);
    nextId++;
  }
  return result;
}

json CxxIndexProvider::membersByName(const std::string& symbol) const {
  auto sym = parseSymbolAt(run("at", symbol));
  std::string kind = sym.value("kind", "class");
  return parseMembersGraph(run("members", symbol), symbol, kind);
}

json CxxIndexProvider::members(const std::string& file, int line, int col) const {
  auto sym = symbolAt(file, line, col);
  if (sym.empty()) return {};
  std::string name = sym.value("name", "");
  std::string kind = sym.value("kind", "class");
  if (name.empty()) return {};
  return parseMembersGraph(run("members", name), name, kind);
}

// ---------------------------------------------------------------------------
// Stats — cxxidx stats <index>
// ---------------------------------------------------------------------------

json CxxIndexProvider::parseStats(const std::string& output) const {
  json result;
  result["indexPath"]  = _indexPath;
  result["available"]  = true;
  result["files"]      = 0;
  result["nodes"]      = 0;
  result["edges"]      = 0;
  result["occurrences"] = 0;

  std::istringstream ss(output);
  std::string line;
  while (std::getline(ss, line)) {
    auto parseCount = [&](const std::string& prefix) -> int {
      if (line.rfind(prefix, 0) == 0) {
        try { return std::stoi(line.substr(prefix.size())); }
        catch (...) {}
      }
      return -1;
    };
    int v;
    if ((v = parseCount("Files:")) >= 0)       result["files"]       = v;
    if ((v = parseCount("Nodes:")) >= 0)       result["nodes"]       = v;
    if ((v = parseCount("Edges:")) >= 0)       result["edges"]       = v;
    if ((v = parseCount("Occurrences:")) >= 0) result["occurrences"] = v;
  }
  return result;
}

json CxxIndexProvider::status() const {
  if (!isAvailable()) {
    json r;
    r["available"] = false;
    r["indexPath"] = _indexPath;
    return r;
  }
  return parseStats(run("stats"));
}
