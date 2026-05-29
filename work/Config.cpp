#include "Config.h"
#include "Utils.h"
#include <filesystem>
#include <fstream>
#include "fmt/color.h"
#include "fmt/format.h"

namespace fs = std::filesystem;

Config::Config() {}

void Config::load(const std::string& configFile,
                  const std::map<std::string, std::string>& overrides) {
  // Priority: command line > config.json > built-in defaults
  json defaults = json::parse(defaultConfigJson());
  json data = defaults;

  if (fileExists(configFile)) {
    json user = json::parse(readFile(configFile));
    data.merge_patch(user);
  }

  // Apply command line overrides (highest priority)
  for (auto& [key, val] : overrides) {
    if (key == "port")
      data[key] = std::stoi(val);
    else if (key == "update-index")
      data[key] = (val == "true");
    else
      data[key] = val;
  }

  url = data["url"];
  port = data["port"];
  htmlRoot = data["root"];
  foldersPage = data["folders"];
  editorPage = data["editor"];
  indexPage = data["index"];
  historyFolderPage = data["history-folder"];
  historyFilePage = data["history-file"];
  diffPage = data["diff"];
  annotatePage = data["annotate"];
  searchPage = data["search"];
  lsp = data["lsp"];
  git = data["git"];
  updateIndex = data["update-index"];
  indexTimestamp = data["index-timestamp"];
  symbolProvider = data["symbol-provider"];
  cxxidx = data.value("cxxidx", "cxxidx");
#ifdef USE_LUCY
  searchEngine = data["search-engine"];
#else
  searchEngine = "grep";
#endif
  theme = data["theme"];

  // Resolve relative paths to absolute (before cwd changes)
  auto resolve = [](const std::string& path) -> std::string {
    return fs::weakly_canonical(fs::path(path)).string();
  };

  htmlRoot = resolve(htmlRoot);
  indexTimestamp = resolve(indexTimestamp);

  if (data.contains("projects")) {
    for (auto& proj : data["projects"]) {
      ProjectInfo info;
      info.name = proj["name"];
      info.source = resolve(proj["source"]);
      info.index = resolve(proj["index"]);
      if (proj.contains("cxxindex") && !proj["cxxindex"].get<std::string>().empty())
        info.cxxindex = resolve(proj["cxxindex"]);
      projects[info.name] = info;
    }
  }

  if (data.contains("cpp-suffix")) {
    cppSuffix.clear();
    for (auto& s : data["cpp-suffix"]) {
      cppSuffix.insert(s.get<std::string>());
    }
  }
}

// Single source of truth for all default configuration values.
// Equivalent to the old ConstStrings.h CONFIG_JSON.
const char* Config::defaultConfigJson() {
  return R"json({
    "url": "http://localhost:3000",
    "port": 3000,
    "root": "root",
    "folders": "folders.html",
    "editor": "editor.html",
    "index": "index.html",
    "history-folder": "history-folder.html",
    "history-file": "history-file.html",
    "diff": "diff.html",
    "search": "search.html",
    "annotate": "annotate.html",
    "lsp": "clangd",
    "git": "/usr/bin/git",
    "cpp-suffix": ["cpp", "c", "h", "hpp", "cxx", "hxx", "cc"],
    "update-index": true,
    "index-timestamp": "index/index.timestamp",
    "symbol-provider": "ctags",
    "search-engine": "lucy",
    "theme": "dark",
    "projects": [
        {
            "name": "CodeBrowser_old",
            "source": "/home/gwerners/projects_claude/CodeBrowser_old",
            "index": "index/CodeBrowser_old-index"
        },
        {
            "name": "lucy-clownfish",
            "source": "/home/gwerners/projects_claude/CodeBrowser_old/lucy-clownfish/compiler",
            "index": "index/lucy-clownfish-index"
        }
    ]
})json";
}
