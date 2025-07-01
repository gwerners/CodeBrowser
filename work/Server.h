#pragma once
#include <list>
#include <map>
#include <set>
#include <string>
#include "FileInfo.h"
#include "Search.h"

#define SOL_USE_MINILUA_HPP_I_ SOL_ON
#define SOL_USING_MINILUA_HPP_I_ SOL_ON
#define SOL_ALL_SAFETIES_ON 1
#include <sol/sol.hpp>

typedef std::map<std::string, std::pair<std::string, std::string>> projectMap;

class ServerPrivData {
 public:
  ServerPrivData()
      : _useSearch(false), _isDiff(false), _useGit(false), _isBlame(false) {}
  bool _useSearch;
  bool _isDiff;
  bool _useGit;
  bool _isBlame;
  std::string _project;
  std::string _filename;
  std::string _root;
  std::string _path;
  std::string _hash;
  std::string _line;
  std::string _r1;
  std::string _r2;
  std::string _fullTextQuery;
  bool _updateIndex;
};
class Server {
 public:
  Server();
  void configure();
  void updateIndexes();
  void addGitResult(sol::state& lua, const std::string path);
  void addFileInfo(sol::state& lua,
                   const std::string& root,
                   const std::string& path);
  void addSearchInfo(sol::state& lua);
  std::string load(const ServerPrivData& priv);
  std::string processLuaTemplate(std::string& htmlLuaTemplate, sol::state& lua);
  void run();

 private:
  projectMap _projects;
  std::set<std::string> _openFiles;
  std::string _url;
  int _port;
  std::string _htmlRoot;
  std::string _folders;
  std::string _editor;
  std::string _index;
  std::string _historyFolder;
  std::string _historyFile;
  std::string _diff;
  std::string _annotate;
  std::string _lsp;
  std::string _git;
  std::string _search;
  bool _updateIndex;
  std::vector<SearchInfo> _searchInfo;
  std::set<std::string> _cppSuffix;
};
