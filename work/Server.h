#pragma once
#include <atomic>
#include <map>
#include <memory>
#include <mutex>
#include <set>
#include <string>
#include <thread>
#include <vector>
#include "Config.h"
#include "FileInfo.h"
#include "Search.h"
#include "crow.h"
#include "ctags/CtagsProvider.h"
#include "cxxindex/CxxIndexProvider.h"

#define SOL_USE_MINILUA_HPP_I_ SOL_ON
#define SOL_USING_MINILUA_HPP_I_ SOL_ON
#define SOL_ALL_SAFETIES_ON 1
#include <sol/sol.hpp>

// Request context extracted from URL parameters
// Per-project background indexing status
struct IndexStatus {
  std::atomic<bool>        running{false};
  std::vector<std::string> log;      // rolling tail of cxxidx output
  int                      exitCode = -1;
  std::mutex               mutex;
};

struct RequestContext {
  std::string project;
  std::string root;       // project source root + path
  std::string path;       // relative path within project
  std::string hash;       // git commit hash
  std::string line;       // line number
  std::string r1, r2;     // diff revisions
  std::string fullTextQuery;
  std::string regexpQuery;
  std::string templateFile;
  std::string indexCreationTime;
  bool useSearch = false;
  bool isDiff = false;
  bool useGit = false;
  bool isBlame = false;
};

class Server {
 public:
  Server(const std::string& configFile = "config.json",
         const std::map<std::string, std::string>& overrides = {});
  void run();

 private:
  // Initialization
  void updateIndexes();
  void updateCtagsIndexes();

  // Template processing
  std::string renderTemplate(const RequestContext& ctx);
  std::string processLuaTemplate(std::string& html, sol::state& lua);

  // Lua helpers - register data with Lua state
  void registerProjects(sol::state& lua);
  void registerGitCommits(sol::state& lua, const std::string& path);
  void registerFileInfo(sol::state& lua,
                        const std::string& root,
                        const std::string& path);
  void registerSearchInfo(sol::state& lua);

  // Route helpers
  RequestContext extractParams(const crow::request& req);
  std::string resolveFileSuffix(const std::string& path);

  Config _config;
  std::set<std::string> _openFiles;
  std::vector<SearchInfo> _searchInfo;
  std::map<std::string, CtagsProvider>                       _ctagsProviders;
  std::map<std::string, CxxIndexProvider>                    _cxxProviders;
  std::map<std::string, std::shared_ptr<IndexStatus>>        _indexStatus;
};
