#include "indexer/GrepSearcher.h"
#include <algorithm>
#include <sstream>
#include "PipeCommand.h"
#include "fmt/color.h"
#include "fmt/format.h"

static bool s_ripgrepChecked = false;
static bool s_hasRipgrep = false;

bool GrepSearcher::hasRipgrep() {
  if (!s_ripgrepChecked) {
    std::string out = PipeCommand::cmd("which", "rg");
    s_hasRipgrep = !out.empty();
    s_ripgrepChecked = true;
  }
  return s_hasRipgrep;
}

GrepSearcher::GrepSearcher(const std::string& sourceDir)
    : _sourceDir(sourceDir), _useRipgrep(hasRipgrep()) {}

std::vector<GrepResult> GrepSearcher::search(const std::string& query,
                                             const std::string& regexp) {
  std::vector<GrepResult> results;
  if (query.empty() || _sourceDir.empty())
    return results;

  std::string output;

  // Search source code files only, word boundary match
  if (_useRipgrep) {
    output = PipeCommand::cmd(
        "rg", "-n", "--no-heading", "-w",
        "-g", "*.{cpp,c,h,hpp,cxx,hxx,cc,js,py,java,go,rs}",
        query.c_str(), _sourceDir.c_str());
  } else {
    output = PipeCommand::cmd(
        "grep", "-rnw",
        "--include=*.{cpp,c,h,hpp,cxx,hxx,cc,js,py,java,go,rs}",
        query.c_str(), _sourceDir.c_str());
  }

  if (output.empty())
    return results;

  // Parse output: /absolute/path/to/file:line:content
  size_t rootLen = _sourceDir.size();
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
    std::string lineNum = line.substr(firstColon + 1,
                                      secondColon - firstColon - 1);
    std::string content = line.substr(secondColon + 1);

    // Apply regexp filter if provided
    if (!regexp.empty()) {
      if (content.find(regexp) == std::string::npos)
        continue;
    }

    // Make path relative to source dir
    std::string relPath = file;
    if (relPath.size() > rootLen && relPath.substr(0, rootLen) == _sourceDir) {
      relPath = relPath.substr(rootLen);
      if (!relPath.empty() && relPath[0] == '/')
        relPath = relPath.substr(1);
    }

    // Trim leading whitespace from content
    auto start = content.find_first_not_of(" \t");
    if (start != std::string::npos)
      content = content.substr(start);

    GrepResult r;
    r.line = lineNum;
    r.url = "/" + relPath;
    r.excerpt = content;
    results.push_back(std::move(r));
  }

  std::sort(results.begin(), results.end());
  return results;
}
