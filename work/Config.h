#pragma once
#include <map>
#include <set>
#include <string>
#include <vector>
#include "nlohmann/json.hpp"

using json = nlohmann::json;

struct ProjectInfo {
  std::string name;
  std::string source;
  std::string index;      // full-text search index directory
  std::string cxxindex;   // path to .cxxi file (cxx-index)
};

class Config {
 public:
  Config();
  void load(const std::string& configFile = "config.json",
            const std::map<std::string, std::string>& overrides = {});

  std::string url;
  int port = 0;
  std::string htmlRoot;
  std::string lsp;
  std::string git;
  bool updateIndex = false;
  std::string indexTimestamp;
  std::string indexCreationTime;
  std::set<std::string> cppSuffix;
  std::map<std::string, ProjectInfo> projects;

  // HTML template filenames
  std::string foldersPage;
  std::string editorPage;
  std::string indexPage;
  std::string historyFolderPage;
  std::string historyFilePage;
  std::string diffPage;
  std::string annotatePage;
  std::string searchPage;

  // Symbol provider: "clangd", "ctags", or "cxxlsp"
  std::string symbolProvider;

  // Path to cxxidx binary (used when symbol-provider is "cxxlsp")
  std::string cxxidx;

  // Search engine: "lucy" or "grep"
  std::string searchEngine;

  // Theme: "light" or "dark"
  std::string theme;

  static const char* defaultConfigJson();
};
