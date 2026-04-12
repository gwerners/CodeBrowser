#include "ctags/CtagsProvider.h"
#include <climits>
#include <fstream>
#include <sstream>
#include "PipeCommand.h"
#include "Utils.h"
#include "fmt/color.h"
#include "fmt/format.h"

CtagsProvider::CtagsProvider() {}

bool CtagsProvider::index(const std::string& sourceDir) {
  _sourceDir = sourceDir;
  std::string tagsFile = sourceDir + "/tags";

  // Run Universal Ctags with extended fields
  // --fields=+lnKS: line number, kind (long), signature
  // --extras=+q: qualified tags (Class::method)
  // Universal Ctags flags:
  //   --fields=+lnKS  : line number, kind (long name), signature
  //   --extras=+q      : qualified tags (Class::method)
  //   -f <file>        : output file
  std::string output = PipeCommand::cmd(
      "ctags", "-R", "--fields=+lnKS", "--extras=+q",
      "--languages=C,C++,Java,Python,JavaScript,Go,Rust",
      "-f", tagsFile.c_str(),
      sourceDir.c_str());

  fmt::print(fg(fmt::color::green), "ctags indexed: {}\n", sourceDir);
  return loadTags(tagsFile);
}

bool CtagsProvider::loadTags(const std::string& tagsFile) {
  std::ifstream file(tagsFile);
  if (!file.is_open()) {
    fmt::print(fg(fmt::color::red), "Cannot open tags file: {}\n", tagsFile);
    return false;
  }

  _symbols.clear();
  _fileIndex.clear();

  std::string line;
  int count = 0;
  while (std::getline(file, line)) {
    if (line.empty() || line[0] == '!')
      continue;  // skip meta lines
    if (parseLine(line))
      count++;
  }

  _loaded = true;
  fmt::print(fg(fmt::color::green), "Loaded {} symbols from {}\n", count,
             tagsFile);
  return true;
}

bool CtagsProvider::parseLine(const std::string& line) {
  // ctags format: name\tfile\tpattern;"\tkind\tfield:value\t...
  auto parts = split(line, '\t');
  if (parts.size() < 3)
    return false;

  CtagsEntry entry;
  entry.name = parts[0];
  entry.file = parts[1];

  // Parse extended fields
  // Field 3 is the kind (e.g. "function", "class", "struct", "variable", "f", "c")
  // Fields 4+ are key:value extended fields
  if (parts.size() > 3) {
    entry.kind = parts[3];
  }

  for (size_t i = 4; i < parts.size(); i++) {
    const auto& field = parts[i];

    if (field.substr(0, 5) == "line:") {
      try {
        entry.line = std::stoi(field.substr(5));
      } catch (...) {
      }
    } else if (field.substr(0, 5) == "kind:") {
      entry.kind = field.substr(5);
    } else if (field.substr(0, 10) == "signature:") {
      entry.signature = field.substr(10);
    } else if (field.substr(0, 6) == "class:" ||
               field.substr(0, 7) == "struct:" ||
               field.substr(0, 10) == "namespace:") {
      auto colonPos = field.find(':');
      entry.scope = field.substr(colonPos + 1);
    }
  }

  // If no line from extended fields, try to parse from pattern
  if (entry.line == 0 && parts.size() >= 3) {
    // Pattern might be /^...$/;" or just a line number
    const auto& pattern = parts[2];
    if (!pattern.empty() && pattern[0] != '/') {
      try {
        entry.line = std::stoi(pattern);
      } catch (...) {
      }
    }
  }

  _symbols.insert({entry.name, entry});
  _fileIndex.insert({entry.file, entry});
  return true;
}

std::vector<CtagsEntry> CtagsProvider::findDefinition(
    const std::string& symbol) const {
  std::vector<CtagsEntry> results;
  auto range = _symbols.equal_range(symbol);
  for (auto it = range.first; it != range.second; ++it) {
    results.push_back(it->second);
  }
  return results;
}

std::vector<CtagsEntry> CtagsProvider::findSymbolsInFile(
    const std::string& file) const {
  std::vector<CtagsEntry> results;
  auto range = _fileIndex.equal_range(file);
  for (auto it = range.first; it != range.second; ++it) {
    results.push_back(it->second);
  }
  return results;
}

CtagsEntry CtagsProvider::findSymbolAtLine(const std::string& file,
                                           int line) const {
  CtagsEntry best;
  int bestDist = INT_MAX;

  auto range = _fileIndex.equal_range(file);
  for (auto it = range.first; it != range.second; ++it) {
    int dist = std::abs(it->second.line - line);
    if (dist < bestDist) {
      bestDist = dist;
      best = it->second;
    }
  }
  return best;
}

std::string CtagsProvider::getHoverInfo(const std::string& symbol) const {
  auto entries = findDefinition(symbol);
  if (entries.empty())
    return "";

  const auto& e = entries[0];
  std::string info;

  if (!e.kind.empty())
    info += "[" + e.kind + "] ";

  if (!e.scope.empty())
    info += e.scope + "::";

  info += e.name;

  if (!e.signature.empty())
    info += e.signature;

  info += "\n\nDefined in: " + e.file + ":" + std::to_string(e.line);

  return info;
}

static json entryToJson(const CtagsEntry& e,
                        const std::string& projectRoot,
                        const std::string& projectName,
                        const std::string& serverUrl) {
  std::string relPath = e.file;
  if (relPath.find(projectRoot) == 0) {
    relPath = relPath.substr(projectRoot.size());
    if (!relPath.empty() && relPath[0] == '/')
      relPath = relPath.substr(1);
  }
  std::string uri = fmt::format("{}/files?project={}&path={}&line={}",
                                serverUrl, projectName, relPath, e.line);
  return {{"uri", uri}};
}

static bool isTypeKind(const std::string& kind) {
  return kind == "class" || kind == "struct" || kind == "enum" ||
         kind == "typedef" || kind == "union" || kind == "interface" ||
         kind == "c" || kind == "s" || kind == "g" || kind == "t" || kind == "u";
}

static bool isFunctionKind(const std::string& kind) {
  return kind == "function" || kind == "method" || kind == "prototype" ||
         kind == "f" || kind == "m" || kind == "p";
}

json CtagsProvider::definitionToJson(const std::string& symbol,
                                     const std::string& projectRoot,
                                     const std::string& projectName,
                                     const std::string& serverUrl) const {
  auto entries = findDefinition(symbol);
  if (entries.empty())
    return json{};
  return entryToJson(entries[0], projectRoot, projectName, serverUrl);
}

json CtagsProvider::typeDefinitionToJson(const std::string& symbol,
                                         const std::string& projectRoot,
                                         const std::string& projectName,
                                         const std::string& serverUrl) const {
  auto entries = findDefinition(symbol);
  // Find first entry that is a type (class/struct/enum/typedef)
  for (auto& e : entries) {
    if (isTypeKind(e.kind))
      return entryToJson(e, projectRoot, projectName, serverUrl);
  }
  // Fallback to first entry
  if (!entries.empty())
    return entryToJson(entries[0], projectRoot, projectName, serverUrl);
  return json{};
}

json CtagsProvider::implementationToJson(const std::string& symbol,
                                          const std::string& projectRoot,
                                          const std::string& projectName,
                                          const std::string& serverUrl) const {
  auto entries = findDefinition(symbol);
  // Find first function/method entry (not prototype/declaration)
  // Prefer entries in .cpp/.c files over .h files
  const CtagsEntry* best = nullptr;
  for (auto& e : entries) {
    if (!isFunctionKind(e.kind))
      continue;
    if (!best) {
      best = &e;
      continue;
    }
    // Prefer .cpp over .h (implementation over declaration)
    bool bestIsHeader = endsWith(best->file, ".h") || endsWith(best->file, ".hpp");
    bool eIsHeader = endsWith(e.file, ".h") || endsWith(e.file, ".hpp");
    if (bestIsHeader && !eIsHeader)
      best = &e;
  }
  if (best)
    return entryToJson(*best, projectRoot, projectName, serverUrl);
  // Fallback
  if (!entries.empty())
    return entryToJson(entries[0], projectRoot, projectName, serverUrl);
  return json{};
}

json CtagsProvider::hoverToJson(const std::string& symbol) const {
  std::string info = getHoverInfo(symbol);
  if (info.empty())
    return json{};
  return {{"id", 1}, {"jsonrpc", "2.0"}, {"description", info}};
}

// Parse grep/rg output lines (file:line:content) into JSON references
static json parseGrepOutput(const std::string& output,
                            const std::string& projectRoot,
                            const std::string& projectName,
                            const std::string& serverUrl) {
  json refs = json::array();
  if (output.empty())
    return refs;

  std::istringstream stream(output);
  std::string line;
  while (std::getline(stream, line)) {
    if (line.empty())
      continue;

    auto firstColon = line.find(':');
    if (firstColon == std::string::npos)
      continue;
    auto secondColon = line.find(':', firstColon + 1);
    if (secondColon == std::string::npos)
      continue;

    std::string file = line.substr(0, firstColon);
    std::string lineNumStr =
        line.substr(firstColon + 1, secondColon - firstColon - 1);

    int lineNum = 0;
    try {
      lineNum = std::stoi(lineNumStr);
    } catch (...) {
      continue;
    }

    std::string relPath = file;
    if (relPath.find(projectRoot) == 0) {
      relPath = relPath.substr(projectRoot.size());
      if (!relPath.empty() && relPath[0] == '/')
        relPath = relPath.substr(1);
    }

    std::string uri = fmt::format("{}/files?project={}&path={}&line={}",
                                  serverUrl, projectName, relPath, lineNum);
    std::string filename =
        file.substr(file.find_last_of("/\\") + 1) + ":" + std::to_string(lineNum);
    refs.push_back({{"name", filename}, {"url", uri}});
  }
  return refs;
}

json CtagsProvider::referencesToJson(const std::string& symbol,
                                     const std::string& projectRoot,
                                     const std::string& projectName,
                                     const std::string& serverUrl) const {
  if (symbol.empty() || _sourceDir.empty())
    return json::array();

  // Build word-boundary pattern for the symbol
  std::string pattern = "\\b" + symbol + "\\b";
  std::string output;

  // Try ripgrep first (faster, respects .gitignore)
  output = PipeCommand::cmd("rg", "-n", "--no-heading",
                            pattern.c_str(), _sourceDir.c_str());

  // Fallback to grep if rg returned nothing (might not be installed)
  if (output.empty()) {
    output = PipeCommand::cmd("grep", "-rnw", symbol.c_str(),
                              _sourceDir.c_str());
  }

  return parseGrepOutput(output, projectRoot, projectName, serverUrl);
}
