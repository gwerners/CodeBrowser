#include "Server.h"
#include <filesystem>
#include <fstream>
#include <thread>
#include "Git.h"
#include "Utils.h"
#include "crow.h"
#include "fmt/color.h"
#include "fmt/format.h"
#include "indexer/GrepSearcher.h"
#ifdef USE_LUCY
#include "indexer/FullTextIndexer.h"
#include "indexer/FullTextSearcher.h"
#endif
#include "lsp/LspClient.h"

namespace fs = std::filesystem;
using json = nlohmann::json;

// --- Static helpers ---

static void serveStaticFile(const std::string& filename, crow::response& res) {
  std::ifstream file(filename, std::ios::binary | std::ios::ate);
  if (!file) {
    res.code = 404;
    res.write("File not found");
    res.end();
    return;
  }

  auto fileSize = file.tellg();
  file.seekg(0, std::ios::beg);
  res.set_header("Content-Length", std::to_string(fileSize));

  // MIME types
  static const std::map<std::string, std::string> mimeTypes = {
      {".html", "text/html"},
      {".css", "text/css"},
      {".js", "application/javascript"},
      {".json", "application/json"},
      {".png", "image/png"},
      {".jpg", "image/jpeg"},
      {".jpeg", "image/jpeg"},
      {".ico", "image/x-icon"},
      {".gif", "image/gif"},
      {".svg", "image/svg+xml"},
      {".woff", "font/woff"},
      {".woff2", "font/woff2"},
      {".ttf", "font/ttf"},
      {".map", "application/json"},
  };

  for (auto& [ext, mime] : mimeTypes) {
    if (endsWith(filename, ext)) {
      res.set_header("Content-Type", mime);
      break;
    }
  }

  std::string content;
  char buffer[8192];
  while (file) {
    file.read(buffer, sizeof(buffer));
    auto n = file.gcount();
    if (n > 0)
      content.append(buffer, n);
  }
  replace_invalid_utf8(content);
  res.write(content);
  res.end();
}

// --- Server implementation ---

Server::Server(const std::string& configFile,
               const std::map<std::string, std::string>& overrides) {
  _config.load(configFile, overrides);
  updateIndexes();
  updateCtagsIndexes();
}

void Server::updateIndexes() {
#ifdef USE_LUCY
  if (_config.searchEngine != "lucy")
    return;

  // Check if any index is missing or empty (no snapshot = not indexed)
  for (auto& [name, proj] : _config.projects) {
    std::string snapshot = proj.index + "/snapshot_2.json";
    if (!fileExists(snapshot)) {
      _config.updateIndex = true;
      break;
    }
  }

  if (_config.updateIndex) {
    fmt::print(fg(fmt::color::green), "Started Lucy index update\n");
    for (auto& [name, proj] : _config.projects) {
      fmt::print(fg(fmt::color::green), "Indexing [{}]: {} -> {}\n", name,
                 proj.source, proj.index);
      FullTextIndexer indexer(proj.index, proj.source);
      indexer.update();
    }
    saveTimestampToJson(_config.indexTimestamp);
    fmt::print(fg(fmt::color::green), "Finished Lucy index update\n");
  }
  _config.indexCreationTime = loadTimestampFromJson(_config.indexTimestamp);
#else
  fmt::print(fg(fmt::color::yellow),
             "Lucy not compiled - using grep/ripgrep for search\n");
#endif
}

void Server::updateCtagsIndexes() {
  // Always load ctags - used for document symbols regardless of symbol-provider
  fmt::print(fg(fmt::color::green), "Building ctags indexes...\n");
  for (auto& [name, proj] : _config.projects) {
    CtagsProvider provider;
    std::string tagsFile = proj.source + "/tags";
    if (fileExists(tagsFile)) {
      provider.setSourceDir(proj.source);
      provider.loadTags(tagsFile);
    } else {
      provider.index(proj.source);
    }
    _ctagsProviders[name] = std::move(provider);
    fmt::print(fg(fmt::color::green), "ctags [{}]: {} symbols\n", name,
               _ctagsProviders[name].symbolCount());
  }
}

RequestContext Server::extractParams(const crow::request& req) {
  RequestContext ctx;
  ctx.indexCreationTime = _config.indexCreationTime;

  auto get = [&](const char* key) -> std::string {
    auto val = req.url_params.get(key);
    return val ? std::string(val) : "";
  };

  ctx.project = get("project");
  if (!ctx.project.empty() && _config.projects.count(ctx.project)) {
    ctx.root = _config.projects[ctx.project].source;
    if (!ctx.root.empty())
      fs::current_path(fs::path(ctx.root));
  }

  ctx.path = get("path");
  if (!ctx.path.empty())
    ctx.root += '/' + ctx.path;

  ctx.hash = get("hash");
  ctx.line = get("line");
  if (ctx.line.empty())
    ctx.line = "0";

  // blame parameter: its presence means "do git blame", even if value is empty (HEAD)
  if (req.url_params.get("blame") != nullptr) {
    ctx.hash = get("blame");
    ctx.isBlame = true;
  }

  ctx.r1 = get("r1");
  ctx.r2 = get("r2");
  ctx.isDiff = (!ctx.r1.empty() && !ctx.r2.empty());

  ctx.fullTextQuery = get("q");
  ctx.regexpQuery = get("r");

  return ctx;
}

std::string Server::resolveFileSuffix(const std::string& path) {
  auto idx = path.rfind('.');
  if (idx == std::string::npos)
    return "txt";
  std::string suffix = path.substr(idx + 1);
  if (_config.cppSuffix.count(suffix))
    return "cpp";
  return suffix;
}

// --- Lua template helpers ---

void Server::registerProjects(sol::state& lua) {
  sol::table tbl = lua.create_table("projects");
  for (auto& [name, proj] : _config.projects) {
    tbl[name] = std::make_pair(proj.source, proj.index);
  }
}

void Server::registerGitCommits(sol::state& lua, const std::string& path) {
  Git git(_config.git);
  auto commits = git.commits(path.empty() ? "." : path);
  lua.new_usertype<Commit>("Commit", "hash", &Commit::hash, "date",
                           &Commit::date, "author", &Commit::author, "message",
                           &Commit::message, "files", &Commit::files);
  lua["commits"] = commits;
}

void Server::registerFileInfo(sol::state& lua,
                              const std::string& root,
                              const std::string& path) {
  lua["suffix"] = resolveFileSuffix(path);

  fs::path p = path;
  lua["upperPath"] = p.parent_path().string();

  lua.new_usertype<FileInfo>("FileInfo", "path", &FileInfo::path, "name",
                             &FileInfo::name, "access_date",
                             &FileInfo::access_date, "size", &FileInfo::size,
                             "type", &FileInfo::type, "isDir", &FileInfo::isDir);

  lua.set_function("get_file_info", [root, path]() {
    std::vector<FileInfo> files;
    if (!fs::exists(root) || !fs::is_directory(root))
      return files;

    for (const auto& entry : fs::directory_iterator(root)) {
      FileInfo info;
      info.path = path;
      info.name = entry.path().filename().string();
      info.isDir = entry.is_directory();
      info.size = info.isDir ? 0 : entry.file_size();
      info.type = info.isDir ? "folders" : "files";

      auto ftime = entry.last_write_time();
      auto tt = to_time_t(ftime);
      auto* gmt = std::gmtime(&tt);
      std::stringstream buf;
      buf << std::put_time(gmt, "%Y/%m/%d %H:%M:%S");
      info.access_date = buf.str();

      files.push_back(info);
    }
    std::sort(files.begin(), files.end());
    return files;
  });
}

void Server::registerSearchInfo(sol::state& lua) {
  lua.new_usertype<SearchLine>("SearchLine", "line", &SearchLine::line,
                               "excerpt", &SearchLine::excerpt);
  lua.new_usertype<SearchFile>("SearchFile", "name", &SearchFile::name, "lines",
                               &SearchFile::lines);
  lua.new_usertype<SearchInfo>("SearchInfo", "path", &SearchInfo::path, "files",
                               &SearchInfo::files);
  lua["searchInfo"] = _searchInfo;
}

std::string Server::renderTemplate(const RequestContext& ctx) {
  std::string html = readFile(ctx.templateFile);

  sol::state lua;
  lua.open_libraries(sol::lib::base);

  registerProjects(lua);
  if (ctx.useGit)
    registerGitCommits(lua, ctx.path);

  lua["serverUrl"] = _config.url;
  lua["serverName"] = "CodeBrowser";
  lua["title"] = "CodeBrowser";
  lua["pageTitle"] = "CodeBrowser";
  lua["project"] = ctx.project;
  lua["indexCreationTime"] = ctx.indexCreationTime;
  lua["githubUrl"] = "https://github.com/gwerners";
  lua["theme"] = _config.theme;
  lua["path"] = ctx.path;
  lua["hash"] = ctx.hash;
  lua["line"] = ctx.line;

  lua["r1"] = ctx.r1;
  lua["r2"] = ctx.r2;

  registerFileInfo(lua, ctx.root, ctx.path);
  if (ctx.useSearch)
    registerSearchInfo(lua);

  lua["fullTextQuery"] = ctx.fullTextQuery;
  lua["regexpTextQuery"] = ctx.regexpQuery;

  return processLuaTemplate(html, lua);
}

std::string Server::processLuaTemplate(std::string& html, sol::state& lua) {
  const std::string BEGIN_TAG = "<!--%lua%";
  const std::string END_TAG = "%lua%-->";
  int index = 1;
  std::size_t pos = 0;

  while (true) {
    auto begin = html.find(BEGIN_TAG, pos);
    if (begin == std::string::npos)
      break;

    auto end = html.find(END_TAG, begin + BEGIN_TAG.length());
    if (end == std::string::npos)
      break;

    std::string luaCode = html.substr(begin + BEGIN_TAG.length(),
                                      end - (begin + BEGIN_TAG.length()));
    std::string fnName = fmt::format("_fn{:06}", index++);
    std::string fullCode = fmt::format("function {}() {}end", fnName, luaCode);

    lua.script(fullCode);

    std::stringstream buffer;
    lua.set_function("print",
                     [&buffer](const std::string& msg) { buffer << msg; });
    lua.script(fmt::format("{}()", fnName));

    std::string output = buffer.str();
    pos = begin + output.length();
    html.replace(begin, (end - begin) + END_TAG.length(), output);
  }

  return html;
}

// --- HTTP Routes ---

void Server::run() {
  fmt::print(fg(fmt::color::green), "Starting server on port {}\n",
             _config.port);
  signal(SIGCHLD, sigchld_handler);

  crow::SimpleApp app;

  // Home page
  CROW_ROUTE(app, "/")
  ([&](const crow::request&) {
    RequestContext ctx;
    ctx.indexCreationTime = _config.indexCreationTime;
    ctx.templateFile = _config.htmlRoot + "/" + _config.indexPage;
    return renderTemplate(ctx);
  });

  // Favicon
  CROW_ROUTE(app, "/favicon.ico")
  ([&](crow::response& res) {
    serveStaticFile(_config.htmlRoot + "/files/favicon.ico", res);
  });

  // Folder browser
  CROW_ROUTE(app, "/folders")
  ([&](const crow::request& req) {
    auto ctx = extractParams(req);
    ctx.templateFile = _config.htmlRoot + "/" + _config.foldersPage;
    return crow::response{renderTemplate(ctx)};
  });

  // File viewer (editor)
  CROW_ROUTE(app, "/files")
  ([&](const crow::request& req) {
    auto ctx = extractParams(req);
    ctx.templateFile = _config.htmlRoot + "/" + _config.editorPage;
    return crow::response{renderTemplate(ctx)};
  });

  // Static files
  CROW_ROUTE(app, "/files/<path>")
      .methods("GET"_method)([&](crow::response& res, std::string path) {
        serveStaticFile(_config.htmlRoot + "/files/" + path, res);
      });

  // Monaco editor assets
  CROW_ROUTE(app, "/monaco/<path>")
      .methods("GET"_method)([&](crow::response& res, std::string path) {
        serveStaticFile(_config.htmlRoot + "/monaco/" + path, res);
      });

  // Raw file content (for editor to fetch)
  CROW_ROUTE(app, "/bin")
  ([&](const crow::request& req) {
    auto ctx = extractParams(req);
    std::string content;

    if (ctx.isBlame) {
      Git git(_config.git);
      content = git.blame(ctx.path, ctx.hash);
    } else if (!ctx.hash.empty()) {
      Git git(_config.git);
      content = git.show(ctx.path, ctx.hash);
    } else {
      content = readFile(ctx.root);
    }

    // Open file in LSP server if it's a C++ file and using clangd
    if (!ctx.isBlame && _config.symbolProvider == "clangd") {
      std::string suffix = resolveFileSuffix(ctx.root);
      if (suffix == "cpp" && _openFiles.find(ctx.root) == _openFiles.end()) {
        LspClient lsp(_config.lsp);
        lsp.didOpen(ctx.root, content);
        _openFiles.insert(ctx.root);
      }
    }

    return crow::response{content};
  });

  // Hover info
  CROW_ROUTE(app, "/hover")
  ([&](const crow::request& req) {
    auto ctx = extractParams(req);
    if (!fileExists(ctx.root))
      return crow::response{"{}"};

    int line = std::stoi(req.url_params.get("line"));
    int column = std::stoi(req.url_params.get("column"));

    json answer;
    if (_config.symbolProvider == "ctags" &&
        _ctagsProviders.count(ctx.project)) {
      // Use ctags for hover
      // Get word at position from the file content
      auto identifier = req.url_params.get("identifier");
      if (identifier)
        answer = _ctagsProviders[ctx.project].hoverToJson(identifier);
      else
        answer = json{};
    } else {
      // Use clangd
      LspClient lsp(_config.lsp);
      answer = lsp.hover(ctx.root, line, column);
    }

    return crow::response{answer.dump()};
  });

  // History (file or folder)
  CROW_ROUTE(app, "/history")
  ([&](const crow::request& req) {
    auto ctx = extractParams(req);
    ctx.useGit = true;
    ctx.templateFile = _config.htmlRoot + "/" +
                       (fs::is_directory(ctx.root) ? _config.historyFolderPage
                                                   : _config.historyFilePage);
    return crow::response{renderTemplate(ctx)};
  });

  // Symbol lookup (definition, declaration, references, etc.)
  CROW_ROUTE(app, "/identifier")
  ([&](const crow::request& req) {
    auto ctx = extractParams(req);
    if (!fileExists(ctx.root))
      return crow::response{"{}"};

    std::string type = req.url_params.get("type") ? req.url_params.get("type") : "";
    std::string identifier =
        req.url_params.get("identifier") ? req.url_params.get("identifier") : "";
    int line = req.url_params.get("line") ? std::stoi(req.url_params.get("line")) : 0;
    int column =
        req.url_params.get("column") ? std::stoi(req.url_params.get("column")) : 0;

    // --- ctags path ---
    if (_config.symbolProvider == "ctags" &&
        _ctagsProviders.count(ctx.project)) {
      auto& ctags = _ctagsProviders[ctx.project];
      const auto& src = _config.projects[ctx.project].source;

      json result;
      if (type == "typeDefinition") {
        result = ctags.typeDefinitionToJson(identifier, src, ctx.project, _config.url);
      } else if (type == "implementation") {
        result = ctags.implementationToJson(identifier, src, ctx.project, _config.url);
      } else {
        result = ctags.definitionToJson(identifier, src, ctx.project, _config.url);
      }

      if (type == "references") {
        json refs = ctags.referencesToJson(
            identifier, _config.projects[ctx.project].source, ctx.project,
            _config.url);
        json resp = {{"references", refs}};
        auto r = crow::response{resp.dump()};
        r.add_header("Content-Type", "application/json");
        return r;
      }
      if (type == "typeHierarchy") {
        json resp = {{"typeHierarchy", json{}}};
        auto r = crow::response{resp.dump()};
        r.add_header("Content-Type", "application/json");
        return r;
      }

      auto r = crow::response{result.dump()};
      r.add_header("Content-Type", "application/json");
      return r;
    }

    // --- clangd path ---
    LspClient lsp(_config.lsp);
    json answer;
    std::string msg;

    // Common lookup types
    if (type == "definition")
      answer = lsp.definition(ctx.root, line, column);
    else if (type == "declaration")
      answer = lsp.declaration(ctx.root, line, column);
    else if (type == "typeDefinition")
      answer = lsp.typeDefinition(ctx.root, line, column);
    else if (type == "implementation")
      answer = lsp.implementation(ctx.root, line, column);
    else if (type == "references") {
      json rawResp = lsp.references(ctx.root, line, column);
      json refs = json::array();
      if (rawResp.contains("result") && rawResp["result"].is_array()) {
        for (auto& entry : rawResp["result"]) {
          std::string targetPath = entry.value("uri", "");
          int targetLine = 0;
          if (entry.contains("range"))
            targetLine = entry["range"]["start"]["line"];

          std::string tmpRoot =
              ctx.root.substr(0, ctx.root.size() - ctx.path.size());
          std::string relPath =
              targetPath.substr(std::string("file://").size() + tmpRoot.size());
          std::string uri =
              fmt::format("{}/files?project={}&path={}&line={}", _config.url,
                          ctx.project, relPath, targetLine + 1);
          std::string filename =
              targetPath.substr(targetPath.find_last_of("/\\") + 1) + ":" +
              std::to_string(targetLine + 1);
          refs.push_back({{"name", filename}, {"url", uri}});
        }
      }
      json resp = {{"references", refs}};
      auto r = crow::response{resp.dump()};
      r.add_header("Content-Type", "application/json");
      return r;
    } else if (type == "typeHierarchy") {
      json thResp = lsp.typeHierarchy(ctx.root, line, column);
      json resp = {{"typeHierarchy", thResp}};
      auto r = crow::response{resp.dump()};
      r.add_header("Content-Type", "application/json");
      return r;
    }

    // For definition/declaration/typeDefinition/implementation:
    // Convert LSP URI to CodeBrowser URL
    if (answer.empty() || !answer.contains("uri"))
      return crow::response{""};

    std::string targetPath = answer["uri"];
    int targetLine = answer.value("line", 0);
    std::string tmpRoot =
        ctx.root.substr(0, ctx.root.size() - ctx.path.size());
    std::string relPath =
        targetPath.substr(std::string("file://").size() + tmpRoot.size());
    std::string uri = fmt::format("{}/files?project={}&path={}&line={}",
                                  _config.url, ctx.project, relPath,
                                  targetLine + 1);

    json resp = {{"uri", uri}};
    auto r = crow::response{resp.dump()};
    r.add_header("Content-Type", "application/json");
    return r;
  });

  // Document symbols (outline) for a file
  // Always uses ctags (works regardless of symbol-provider setting)
  CROW_ROUTE(app, "/symbols")
  ([&](const crow::request& req) {
    auto ctx = extractParams(req);
    json symbols = json::array();

    if (_ctagsProviders.count(ctx.project)) {
      auto entries = _ctagsProviders[ctx.project].findSymbolsInFile(ctx.root);
      for (auto& e : entries) {
        symbols.push_back({
            {"name", e.name},
            {"kind", e.kind},
            {"line", e.line},
            {"signature", e.signature},
            {"scope", e.scope},
        });
      }
    }

    auto r = crow::response{symbols.dump()};
    r.add_header("Content-Type", "application/json");
    return r;
  });

  // Diff view
  CROW_ROUTE(app, "/diff")
  ([&](const crow::request& req) {
    auto ctx = extractParams(req);
    ctx.templateFile = _config.htmlRoot + "/" + _config.diffPage;
    return crow::response{renderTemplate(ctx)};
  });

  // Annotate (git blame)
  CROW_ROUTE(app, "/annotate")
  ([&](const crow::request& req) {
    auto ctx = extractParams(req);
    ctx.useGit = true;
    ctx.templateFile = _config.htmlRoot + "/" + _config.annotatePage;
    return crow::response{renderTemplate(ctx)};
  });

  // Full-text search
  CROW_ROUTE(app, "/search")
  ([&](const crow::request& req) {
    auto ctx = extractParams(req);

    if (!ctx.project.empty() && !ctx.fullTextQuery.empty() &&
        _config.projects.count(ctx.project)) {

      struct RawResult {
        std::string line, url, excerpt;
        bool operator<(const RawResult& o) const { return url < o.url; }
      };
      std::vector<RawResult> rawResults;

#ifdef USE_LUCY
      if (_config.searchEngine == "lucy" &&
          fileExists(_config.projects[ctx.project].index)) {
        FullTextSearcher searcher(_config.projects[ctx.project].index);
        auto results = searcher.search(ctx.fullTextQuery, ctx.regexpQuery);
        for (auto& r : results)
          rawResults.push_back({r.line, r.url, r.excerpt});
      } else
#endif
      {
        GrepSearcher searcher(_config.projects[ctx.project].source);
        auto results = searcher.search(ctx.fullTextQuery, ctx.regexpQuery);
        for (auto& r : results)
          rawResults.push_back({r.line, r.url, r.excerpt});
      }

      // Group results by directory and file
      std::map<std::string, std::map<std::string, std::vector<SearchLine>>>
          grouped;
      for (auto& entry : rawResults) {
        fs::path p = entry.url;
        std::string filename = p.filename().string();
        std::string dir = p.parent_path().string();
        std::string excerpt = entry.excerpt;
        excerpt.erase(std::remove(excerpt.begin(), excerpt.end(), '\n'),
                      excerpt.end());
        excerpt.erase(std::remove(excerpt.begin(), excerpt.end(), '\r'),
                      excerpt.end());
        grouped[dir][filename].push_back({entry.line, excerpt});
      }

      _searchInfo.clear();
      for (auto& [dir, files] : grouped) {
        SearchInfo info;
        info.path = dir;
        if (!info.path.empty() && info.path[0] == '/')
          info.path.erase(0, 1);
        for (auto& [fname, lines] : files) {
          SearchFile sf;
          sf.name = fname;
          sf.lines = lines;
          info.files.push_back(std::move(sf));
        }
        _searchInfo.push_back(std::move(info));
      }
      ctx.useSearch = true;
    }

    ctx.useGit = true;
    ctx.templateFile = _config.htmlRoot + "/" + _config.searchPage;
    return crow::response{renderTemplate(ctx)};
  });

  // Switch header/source
  CROW_ROUTE(app, "/header_source")
  ([&](const crow::request& req) {
    auto ctx = extractParams(req);
    if (!fileExists(ctx.root))
      return crow::response{"{}"};

    if (_config.symbolProvider == "ctags") {
      // For ctags, simply swap .h <-> .cpp extension
      std::string target = ctx.root;
      if (endsWith(target, ".h"))
        target = target.substr(0, target.size() - 2) + ".cpp";
      else if (endsWith(target, ".hpp"))
        target = target.substr(0, target.size() - 4) + ".cpp";
      else if (endsWith(target, ".cpp"))
        target = target.substr(0, target.size() - 4) + ".h";
      else if (endsWith(target, ".c"))
        target = target.substr(0, target.size() - 2) + ".h";

      std::string tmpRoot =
          ctx.root.substr(0, ctx.root.size() - ctx.path.size());
      std::string relPath = target.substr(tmpRoot.size());
      std::string url = fmt::format("{}/files?project={}&path={}", _config.url,
                                    ctx.project, relPath);
      json resp = {{"url", url}};
      auto r = crow::response{resp.dump()};
      r.add_header("Content-Type", "application/json");
      return r;
    }

    LspClient lsp(_config.lsp);
    json result = lsp.switchSourceHeader(ctx.root);
    if (result.empty() || !result.contains("path"))
      return crow::response{"{}"};

    std::string targetPath = result["path"];
    std::string tmpRoot =
        ctx.root.substr(0, ctx.root.size() - ctx.path.size());
    std::string relPath =
        targetPath.substr(std::string("file://").size() + tmpRoot.size());
    std::string url = fmt::format("{}/files?project={}&path={}", _config.url,
                                  ctx.project, relPath);
    json resp = {{"url", url}};
    auto r = crow::response{resp.dump()};
    r.add_header("Content-Type", "application/json");
    return r;
  });

  app.port(_config.port).multithreaded().run();
}
