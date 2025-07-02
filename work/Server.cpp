#include "Server.h"
#include <fcntl.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>
#include <filesystem>
#include <fstream>
#include "ConstStrings.h"
#include "FullTextIndexer.h"
#include "FullTextSearcher.h"
#include "Git.h"
#include "LSMessage.h"
#include "PipeAdapter.h"
#include "PipeCommand.h"
#include "Utils.h"
#include "crow.h"
#include "fmt/color.h"
#include "fmt/format.h"
#include "nlohmann/json.hpp"
/*
Notes:

css color template:
https://www.shecodes.io/palettes/1313
.first-color {
  background: #ececec;
}

.second-color {
  background: #9fd3c7;
}

.third-color {
  background: #385170;
}

.fourth-color {
  background: #142d4c;
}

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

gamma ray

export CMAKE_PREFIX_PATH=$HOME/Qt/6.7.2/gcc_64/
apt-get install libfontconfig1-dev libfreetype6-dev libx11-dev libxext-dev
libxfixes-dev libxi-dev libxrender-dev libxcb1-dev libx11-xcb-dev
libxcb-glx0-dev libxcb-keysyms1-dev libxcb-image0-dev libxcb-shm0-dev
libxcb-icccm4-dev libxcb-sync0-dev libxcb-xfixes0-dev libxcb-shape0-dev
libxcb-randr0-dev libxcb-render-util0-dev

apt-get install libgl1-mesa-dev libxcb-xkb-dev libxkbcommon-dev


$HOME/Qt/6.7.2/gcc_64/bin/qmake -query
QT_SYSROOT:
QT_INSTALL_PREFIX:/home/gwerners/Qt/6.7.2/gcc_64
QT_INSTALL_ARCHDATA:/home/gwerners/Qt/6.7.2/gcc_64
QT_INSTALL_DATA:/home/gwerners/Qt/6.7.2/gcc_64
QT_INSTALL_DOCS:/home/gwerners/Qt/Docs/Qt-6.7.2
QT_INSTALL_HEADERS:/home/gwerners/Qt/6.7.2/gcc_64/include
QT_INSTALL_LIBS:/home/gwerners/Qt/6.7.2/gcc_64/lib
QT_INSTALL_LIBEXECS:/home/gwerners/Qt/6.7.2/gcc_64/libexec
QT_INSTALL_BINS:/home/gwerners/Qt/6.7.2/gcc_64/bin
QT_INSTALL_TESTS:/home/gwerners/Qt/6.7.2/gcc_64/tests
QT_INSTALL_PLUGINS:/home/gwerners/Qt/6.7.2/gcc_64/plugins
QT_INSTALL_QML:/home/gwerners/Qt/6.7.2/gcc_64/qml
QT_INSTALL_TRANSLATIONS:/home/gwerners/Qt/6.7.2/gcc_64/translations
QT_INSTALL_CONFIGURATION:
QT_INSTALL_EXAMPLES:/home/gwerners/Qt/Examples/Qt-6.7.2
QT_INSTALL_DEMOS:/home/gwerners/Qt/Examples/Qt-6.7.2
QT_HOST_PREFIX:/home/gwerners/Qt/6.7.2/gcc_64
QT_HOST_DATA:/home/gwerners/Qt/6.7.2/gcc_64
QT_HOST_BINS:/home/gwerners/Qt/6.7.2/gcc_64/bin
QT_HOST_LIBEXECS:/home/gwerners/Qt/6.7.2/gcc_64/libexec
QT_HOST_LIBS:/home/gwerners/Qt/6.7.2/gcc_64/lib
QMAKE_SPEC:linux-g++
QMAKE_XSPEC:linux-g++
QMAKE_VERSION:3.1
QT_VERSION:6.7.2



to reuse address maybe this is needed (not sure ...)
{socket_.set_option(tcp::socket::reuse_address(true));}
inside SocketAdaptor
crow/crow/socket_adaptors.h:41


opengrok server example
http://bxr.su/

https://cs.android.com/

https://trac.xapian.org/wiki

https://www.youtube.com/watch?v=f6AtGR9RCJ0
https://github.com/boostcon/cppnow_presentations_2023

compile commands
https://github.com/rizsotto/Bear

fulltext search
// https://github.com/apache/lucy-clownfish
// build & install compiler and runtime localy
// cd runtime && cd compiler
//./configure --prefix=/home/gwerners/projects/indexer
// make && make install

// generate include files
//./bin/cfc --include=$PREFIX/share/clownfish/include --parcel=Lucy
//--dest=autogen

// https://github.com/apache/lucy
// cd c
// export PATH=$PATH:/home/gwerners/projects/indexer/bin
//./configure --clownfish-prefix=/home/gwerners/projects/indexer
//--prefix=/home/gwerners/projects/indexer include generated inside
//  /home/gwerners/projects/indexer/lucy-master/c/autogen

//https://lucy.apache.org/docs/c/Lucy/Docs/Tutorial/BeyondSimpleTutorial.html
//https://lucy.apache.org/docs/c/Lucy/Docs/Tutorial/SimpleTutorial.html
//https://lucy.apache.org/docs/c/Clownfish.html
//https://lucy.apache.org/docs/c/Lucy/Docs/Tutorial.html

https://www.jsonrpc.org/
http://www.simple-is-better.org/rpc/#differences-between-1-0-and-2-0


monaco
https://microsoft.github.io/monaco-editor/docs.html
https://microsoft.github.io/monaco-editor/docs.html#interfaces/editor.ICodeEditor.html

Scroll to top, in px:
editor.setScrollPosition({scrollTop: 0});

Scroll to a specific line:
editor.revealLine(15);

Scroll to a specific line so it ends in the center of the editor:
editor.revealLineInCenter(15);

Move current active line:
editor.setPosition({column: 1, lineNumber: 3});



https://microsoft.github.io/monaco-editor/typedoc/enums/KeyCode.html

                editor.onMouseDown((e) => {
                    const { event: { ctrlKey, shiftKey, altKey, metaKey, target
}, target: { position } } = event; console.log(`Mouse clicked at line:
${position.lineNumber}, column: ${position.column}`);

                    if (ctrlKey) {
                        console.log('Ctrl key is pressed.');
                        // Add custom behavior for Ctrl + click
                    }

                    if (shiftKey) {
                        console.log('Shift key is pressed.');
                        // Add custom behavior for Shift + click
                    }

                    if (altKey) {
                        console.log('Alt key is pressed.');
                        // Add custom behavior for Alt + click
                    }

                    if (metaKey) {
                        console.log('Meta key is pressed.');
                        // Add custom behavior for Meta (Command) + click
                    }
                });









                // Register the command in Monaco Editor
                editor.addCommand(monaco.KeyMod.CtrlCmd | monaco.KeyCode.F12,
async () => { const position = editor.getPosition(); const model =
editor.getModel(); const lineNumber = position.lineNumber; const character =
position.column; const word = model.getWordAtPosition(position); if (!word) {
                    return;
                }
                const functionName = word.word;
                try {
<!--%lua%
print("                const definition = await getFunctionDefinitionFile(\"" ..
serverUrl .. "/definition?project=" .. project .. "&path=" .. path .. "\",
functionName, lineNumber, character);\n") %lua%--> if (definition &&
definition.uri) { window.location.href = definition.uri;  // Redirect to the
definition location } else { console.error('Definition not found');
                    }
                } catch (error) {
                    console.error('Error fetching definition:', error);
                }
                });
*/
using json = nlohmann::json;
namespace fs = std::filesystem;

void serveFile(const std::string& filename, crow::response& res) {
  std::ifstream file(filename, std::ios::binary | std::ios::ate);
  if (!file) {
    res.code = 404;
    res.write("File not found");
    res.end();
    return;
  }

  std::ifstream::pos_type fileSize = file.tellg();
  file.seekg(0, std::ios::beg);

  res.set_header("Content-Length", std::to_string(fileSize));

  // Determine MIME type based on file extension
  if (filename.ends_with(".html")) {
    res.set_header("Content-Type", "text/html");
  } else if (filename.ends_with(".css")) {
    res.set_header("Content-Type", "text/css");
  } else if (filename.ends_with(".js")) {
    res.set_header("Content-Type", "application/javascript");
  } else if (filename.ends_with(".json")) {
    res.set_header("Content-Type", "application/json");
  } else if (filename.ends_with(".png")) {
    res.set_header("Content-Type", "image/png");
  } else if (filename.ends_with(".jpg") || filename.ends_with(".jpeg")) {
    res.set_header("Content-Type", "image/jpeg");
  } else if (filename.ends_with(".ico")) {
    res.set_header("Content-Type", "image/x-icon");
  } else if (filename.ends_with(".gif")) {
    res.set_header("Content-Type", "image/gif");
  }  // Add more types as needed

  // Stream file contents
  char buffer[8192];
  std::string out;
  while (file) {
    file.read(buffer, sizeof(buffer));
    std::streamsize bytes_read = file.gcount();
    if (bytes_read > 0) {
      // res.write(std::string(buffer, bytes_read));
      out += std::string(buffer, bytes_read);
    }
  }
  replace_invalid_utf8(out);
  res.write(out);
  res.end();
}

static void fillVariables(projectMap& _projects,
                          const crow::request& req,
                          ServerPrivData& priv) {
  priv._isBlame = false;
  priv._isDiff = false;
  priv._useGit = false;
  priv._useSearch = false;
  if (req.url_params.get("project") != nullptr) {
    priv._project = req.url_params.get("project");
    priv._root = _projects[priv._project].first;
    if (!priv._root.empty()) {
      std::filesystem::current_path(fs::path(priv._root));
    }
  }
  if (req.url_params.get("path") != nullptr) {
    priv._path = req.url_params.get("path");
    priv._root += '/' + priv._path;
  }
  if (req.url_params.get("hash") != nullptr) {
    priv._hash = req.url_params.get("hash");
  }
  if (req.url_params.get("line") != nullptr) {
    priv._line = req.url_params.get("line");
  } else {
    priv._line = "0";
  }
  if (req.url_params.get("blame") != nullptr) {
    priv._hash = req.url_params.get("blame");
    priv._isBlame = true;
  }
  if (req.url_params.get("r1") != nullptr) {
    priv._r1 = req.url_params.get("r1");
  }
  if (req.url_params.get("r2") != nullptr) {
    priv._r2 = req.url_params.get("r2");
  }
  if (!priv._r1.empty() && !priv._r2.empty()) {
    priv._isDiff = true;
  }
  if (req.url_params.get("q") != nullptr) {
    priv._fullTextQuery = req.url_params.get("q");
  }
  if (req.url_params.get("r") != nullptr) {
    priv._regexpTextQuery = req.url_params.get("r");
  }
}
Server::Server() {
  configure();
  updateIndexes();
}
void Server::configure() {
  std::string filename(CONFIG_JSON_FILENAME);
  json data;
  if (exists(filename)) {
    data = json::parse(readFile(filename));
  } else {
    data = json::parse(CONFIG_JSON);
  }
  _url = data["url"];
  _port = data["port"];
  _htmlRoot = data["root"];
  _folders = data["folders"];
  _editor = data["editor"];
  _index = data["index"];
  _historyFolder = data["history-folder"];
  _historyFile = data["history-file"];
  _diff = data["diff"];
  _annotate = data["annotate"];
  _lsp = data["lsp"];
  _git = data["git"];
  _search = data["search"];
  _updateIndex = data["update-index"];
  if (data.contains("projects")) {
    json projects = data["projects"];
    for (size_t pos = 0; pos < projects.size(); ++pos) {
      json project = projects.at(pos);
      // fmt::print("{}\n", project.dump(4));
      std::string key = project["name"];
      std::string value = project["source"];
      std::string index = project["index"];
      _projects.insert({key, {value, index}});
    }
  }
  if (data.contains("cpp-suffix")) {
    json suffix = data["cpp-suffix"];
    for (json::iterator it = suffix.begin(); it != suffix.end(); ++it) {
      if (_cppSuffix.find(*it) == _cppSuffix.end()) {
        std::string s = *it;
        _cppSuffix.insert(*it);
      }
    }
  }
}

void Server::updateIndexes() {
  if (_updateIndex) {
    fmt::print(fmt::emphasis::bold | fg(fmt::color::green),
               START_UPDATE_INDEX_MSG);

    for (auto entry = _projects.begin(); entry != _projects.end(); ++entry) {
      std::string project = entry->first;
      std::string source = entry->second.first;
      std::string index = entry->second.second;
      fmt::print(fmt::emphasis::bold | fg(fmt::color::green), UPDATE_INDEX_MSG,
                 project, index, source);
      FullTextIndexer indexer(index, source);
      indexer.update();
    }
    fmt::print(fmt::emphasis::bold | fg(fmt::color::green),
               FINISHED_UPDATE_INDEX_MSG);
  }
}
void Server::addGitResult(sol::state& lua, const std::string path) {
  Git gitProcess(_git);
  if (path.empty()) {
    return;
  }
  std::vector<Commit> commits = gitProcess.commits(path);
  // Register the Commit struct with Lua
  lua.new_usertype<Commit>("Commit", "hash", &Commit::hash, "date",
                           &Commit::date, "author", &Commit::author, "message",
                           &Commit::message, "files", &Commit::files);

  // Register the vector of commits within Lua
  lua["commits"] = commits;
}
void Server::addFileInfo(sol::state& lua,
                         const std::string& root,
                         const std::string& path) {
  std::string::size_type idx = path.rfind('.');
  std::string suffix;
  if (idx != std::string::npos) {
    suffix = path.substr(idx + 1);
  } else {
    suffix = "txt";
  }
  if (_cppSuffix.find(suffix) != _cppSuffix.end()) {
    suffix = "cpp";
  }
  lua["suffix"] = suffix;

  fs::path p = path;
  std::string upperPath = p.parent_path();
  lua["upperPath"] = upperPath;
  // Define FileInfo as a new usertype in Lua
  lua.new_usertype<FileInfo>(
      "FileInfo", "path", &FileInfo::path, "name", &FileInfo::name,
      "access_date", &FileInfo::access_date, "size", &FileInfo::size, "type",
      &FileInfo::type, "isDir", &FileInfo::isDir);

  // Define a function to retrieve file information from a directory
  lua.set_function("get_file_info",
                   [root, path](/*const std::string& directory*/) {
                     std::vector<FileInfo> files_info;
                     for (const auto& entry : fs::directory_iterator(root)) {
                       // std::cout << entry.path().filename() << std::endl;
                       FileInfo info;
                       std::string localFilename = entry.path().filename();
                       info.path = path;
                       info.name = localFilename;
                       if (entry.is_regular_file()) {
                         info.size = entry.file_size();
                         info.type = "files";
                         info.isDir = false;
                       } else {
                         info.size = 0;
                         info.type = "folders";
                         info.isDir = true;
                       }
                       fs::file_time_type file_time = entry.last_write_time();
                       std::time_t tt = to_time_t(file_time);
                       std::tm* gmt = std::gmtime(&tt);
                       std::stringstream buffer;
                       buffer << std::put_time(gmt, "%Y/%m/%d %H:%M:%S");
                       std::string formattedFileTime = buffer.str();
                       info.access_date = formattedFileTime;

                       files_info.push_back(info);
                     }
                     std::sort(files_info.begin(), files_info.end());
                     return files_info;
                   });
}
void Server::addSearchInfo(sol::state& lua) {
  lua.new_usertype<SearchLine>("SeachLine", "line", &SearchLine::line,
                               "excerpt", &SearchLine::excerpt);
  lua.new_usertype<SearchFile>("SearchFile", "name", &SearchFile::name, "lines",
                               &SearchFile::lines);
  lua.new_usertype<SearchInfo>("SearchInfo", "path", &SearchInfo::path, "files",
                               &SearchInfo::files);

  lua["searchInfo"] = _searchInfo;
}

std::string Server::load(const ServerPrivData& priv) {
  std::string htmlLuaTemplate = readFile(priv._filename);

  sol::state lua;
  lua.open_libraries(sol::lib::base);
  // Open all standard Lua libraries
  /*lua.open_libraries(sol::lib::base, sol::lib::package, sol::lib::coroutine,
                         sol::lib::string, sol::lib::os, sol::lib::math,
                         sol::lib::table, sol::lib::debug, sol::lib::bit32);*/
  // project variables
  sol::table tblProjects = lua.create_table("projects");
  for (auto& entry : _projects) {
    tblProjects[entry.first] = entry.second;
  }

  // Create a vector of commits
  std::cout << "pwd " << std::filesystem::current_path() << std::endl;
  std::cout << "path " << priv._path << std::endl;
  if (priv._useGit) {
    addGitResult(lua, priv._path);
  }
  lua["serverUrl"] = _url;
  lua["serverName"] = "CodeBrowser";
  lua["title"] = "CodeBrowser";
  lua["pageTitle"] = "Gwerners's CodeBrowser";
  lua["project"] = priv._project;
  lua["indexCreationTime"] = "Thu May 09 03:59:59 PDT 2024 (*not updated!)";
  lua["githubUrl"] = "https://github.com/gwerners";
  lua["path"] = priv._path;
  lua["hash"] = priv._hash;
  lua["line"] = priv._line;
  if (priv._isDiff) {
    lua["r1"] = priv._r1;
    lua["r2"] = priv._r2;
  }
  addFileInfo(lua, priv._root, priv._path);
  if (priv._useSearch) {
    addSearchInfo(lua);
  }
  lua["fullTextQuery"] = priv._fullTextQuery;

  lua["regexpTextQuery"] = priv._regexpTextQuery;

  return processLuaTemplate(htmlLuaTemplate, lua);
}

std::string Server::processLuaTemplate(std::string& htmlLuaTemplate,
                                       sol::state& lua) {
  std::string LUA_BEGIN_TAG = "<!--%lua%";
  std::string LUA_END_TAG = "%lua%-->";
  int index = 1;
  std::size_t pos = 0;
  while (true) {
    std::size_t begin = htmlLuaTemplate.find(LUA_BEGIN_TAG, pos);
    std::size_t end;
    if (begin != std::string::npos) {
      // std::cout << "first 'needle' found at: " << begin << '\n';
      end = htmlLuaTemplate.find(LUA_END_TAG, begin + LUA_BEGIN_TAG.length());
      if (begin != std::string::npos) {
        std::size_t start = begin + LUA_BEGIN_TAG.length();
        std::string luaCode = htmlLuaTemplate.substr(start, end - start);
        // std::cout << "lua code [" << luaCode << "]" << std::endl;
        std::string functionName = fmt::format("fnt{0:06}", index++);
        std::string fullCode =
            fmt::format("function {}() {}end", functionName, luaCode);
        // std::cout  << fullCode << std::endl;
        lua.script(fullCode);
        // Create a stringstream to capture output
        std::stringstream buffer;
        // Redirect Lua's print function to write to the stringstream
        lua.set_function("print", [&buffer](const std::string& message) {
          buffer << message;
        });
        lua.script(fmt::format("{}()", functionName));
        // std::cout << "Captured output:\n" << buffer.str() << std::endl;
        pos = begin + buffer.str().length();
        htmlLuaTemplate = htmlLuaTemplate.replace(
            begin, (end - begin) + LUA_END_TAG.length(), buffer.str());
        continue;
      }
    }
    break;
  }
  std::cout << __PRETTY_FUNCTION__ << __LINE__ << std::endl;
  return htmlLuaTemplate;
}
void Server::run() {
  fmt::print(fmt::emphasis::bold | fg(fmt::color::green), STARTING_MSG, _port);

  signal(SIGCHLD, sigchld_handler);
  crow::SimpleApp app;
  // app.loglevel(crow::LogLevel::Debug);

  CROW_ROUTE(app, "/")
  ([&](const crow::request& req) {
    std::cout << "req.url " << req.url << std::endl;
    ServerPrivData priv;
    priv._filename = _htmlRoot + "/" + _index;
    fmt::print(fmt::emphasis::bold | fg(fmt::color::blue), "serving {} \n",
               priv._filename);
    return load(priv);
  });
  CROW_ROUTE(app, "/favicon.ico")
  ([&](crow::response& res) {
    std::string root = _htmlRoot + "/files";
    root += "/favicon.ico";
    std::cout << "serving ico " << root << std::endl;
    serveFile(root, res);
  });
  CROW_ROUTE(app, "/folders")
  ([&](const crow::request& req) {
    std::string body = DEFAULT_BODY;
    ServerPrivData priv;
    fillVariables(_projects, req, priv);
    priv._filename = _htmlRoot + "/" + _folders;
    body = load(priv);
    return crow::response{body};
  });
  CROW_ROUTE(app, "/files")
  ([&](const crow::request& req) {
    std::string body = DEFAULT_BODY;
    ServerPrivData priv;
    fillVariables(_projects, req, priv);
    priv._filename = _htmlRoot + "/" + _editor;
    body = load(priv);
    fmt::print(fmt::emphasis::bold | fg(fmt::color::red), "serving {} \n",
               priv._filename);
    return crow::response{body};
  });

  CROW_ROUTE(app, "/files/<path>")
      .methods("GET"_method)([&](crow::response& res, std::string path) {
        std::string root = _htmlRoot;
        root += "/files/" + path;
        fmt::print(fmt::emphasis::bold | fg(fmt::color::yellow),
                   "file serving {} \n", root);
        serveFile(root, res);
      });
  CROW_ROUTE(app, "/monaco/<path>")
      .methods("GET"_method)([&](crow::response& res, std::string path) {
        std::string root = _htmlRoot;
        root += "/monaco/" + path;
        fmt::print(fmt::emphasis::bold | fg(fmt::color::yellow),
                   "file serving {} \n", root);
        serveFile(root, res);
      });
  CROW_ROUTE(app, "/bin")
  ([&](const crow::request& req) {
    ServerPrivData priv;
    fillVariables(_projects, req, priv);
    std::cout << "pwd " << std::filesystem::current_path() << std::endl;
    std::string content;
    std::string::size_type idx = priv._root.rfind('.');
    std::string extension;
    if (idx != std::string::npos) {
      extension = priv._root.substr(idx + 1);
      std::cout << extension << std::endl;
    }
    if (priv._isBlame) {
      // git blame {commit_id} -- {path/to/file}
      //--date=format:'%Y-%m-%d %H:%M:%S'
      std::cout << "pwd " << std::filesystem::current_path() << std::endl;
      std::cout << ".path " << priv._path << std::endl;
      Git gitProcess(_git);
      content = gitProcess.blame(priv._path, priv._hash);
      // if (hash.empty()) {
      //     content = PipeCommand::cmd(git,"blame","--",path.c_str());
      // } else {
      //     content =
      //     PipeCommand::cmd(git,"blame",hash.c_str(),"--",path.c_str());
      // }
      priv._useGit = true;
      std::cout << "content " << content << std::endl;
    } else {
      if (!priv._hash.empty()) {
        // git show 1234:path/to/file.txt
        // std::string cmd = fmt::format("{}:{}",hash.c_str(), path.c_str());
        // content = PipeCommand::cmd(git,"show",cmd.c_str());
        Git gitProcess(_git);
        content = gitProcess.show(priv._path, priv._hash);
        priv._useGit = true;
      } else {
        content = readFile(priv._root);
        replace_invalid_utf8(content);
      }
    }

    if (!priv._isBlame) {
      if (!extension.empty() &&
          _cppSuffix.find(extension) != _cppSuffix.end()) {
        std::cout << "is cpp source! " << priv._root << std::endl;
        if (auto search = _openFiles.find(priv._root);
            search != _openFiles.end()) {
          std::cout << "Found " << (*search) << '\n';
        } else {
          /*
              std::cout << __PRETTY_FUNCTION__ <<":" <<__LINE__ << std::endl;
              DidClose didClose(lastOpenFile);
              didClose.send();
              msg = didClose.receive();
              std::cout << __PRETTY_FUNCTION__ <<":" <<__LINE__ << std::endl;
              fmt::print(fmt::emphasis::bold | fg(fmt::color::green),
                         "didClose Received from clangd [{}]\n", msg);
          */
          std::string msg;
          DidOpen didOpen(_lsp, priv._root, content);
          didOpen.send();
          msg = didOpen.receive();
          fmt::print(fmt::emphasis::bold | fg(fmt::color::green),
                     "didOpen Received from clangd [{}]\n", msg);
          std::cout << "last file " << priv._root << std::endl;
          static bool once = true;
          if (once) {
            msg = didOpen.receive();
            fmt::print(fmt::emphasis::bold | fg(fmt::color::green),
                       "didOpen Received from clangd [{}]\n", msg);
            once = false;
          }
          _openFiles.insert(priv._root);
        }
      }
    }
    return crow::response{content};
  });

  // https://localhost:3000/hover?word=${word}&line=${lineNumber}&column=${column};
  CROW_ROUTE(app, "/hover")
  ([&](const crow::request& req) {
    // std::cout << "hover  " << std::endl;
    std::string project;
    std::string root;
    if (req.url_params.get("project") != nullptr) {
      project = req.url_params.get("project");
      root = _projects[project].first;
      if (!root.empty()) {
        std::filesystem::current_path(fs::path(root));
      }
    }
    std::string path;
    if (req.url_params.get("path") != nullptr) {
      path = req.url_params.get("path");
      root += '/' + path;
    }
    // std::cout << "root " << root << std::endl;
    // std::cout << "path " << path << std::endl;
    if (!exists(root)) {
      return crow::response{hover_default.dump()};
    }
    int line = std::stoi(req.url_params.get("line"));
    int column = std::stoi(req.url_params.get("column"));
    fmt::print(fmt::emphasis::bold | fg(fmt::color::green),
               "param line[{}] column[{}]\n", line, column);
    std::string content;
    Hover hover(_lsp, root, line, column);
    hover.send();
    std::string msg = hover.receive();
    fmt::print(fmt::emphasis::bold | fg(fmt::color::green),
               "hover Received from clangd [{}]\n", msg);
    json answer = json::parse(msg);
    std::cout << "clang answer only json " << answer.dump(4) << std::endl;
    std::cout << "clang answer " << HoverAnswer::out(answer).dump(4)
              << std::endl;
    return crow::response{HoverAnswer::out(answer).dump()};
  });

  // http://localhost:3000/history?project=dhewm3&path=dist/linux/share/icons/hicolor/scalable/apps/org.dhewm3.Dhewm3.svg
  CROW_ROUTE(app, "/history")
  ([&](const crow::request& req) {
    ServerPrivData priv;
    fillVariables(_projects, req, priv);
    priv._useGit = true;
    if (fs::is_directory(priv._root)) {
      priv._filename = _htmlRoot + "/" + _historyFolder;
    } else {
      priv._filename = _htmlRoot + "/" + _historyFile;
    }
    return crow::response{load(priv)};
  });

  CROW_ROUTE(app, "/identifier")
  ([&](const crow::request& req) {
    std::string project;
    std::string root;
    std::string path;
    std::string type;
    std::string identifier;
    int line = 0;
    int column = 0;
    if (req.url_params.get("project") != nullptr) {
      project = req.url_params.get("project");
      root = _projects[project].first;
      if (!root.empty()) {
        std::filesystem::current_path(fs::path(root));
      }
    }
    if (req.url_params.get("path") != nullptr) {
      path = req.url_params.get("path");
      root += '/' + path;
    }
    if (!exists(root)) {
      return crow::response{"{}"};
    }
    if (req.url_params.get("type") != nullptr) {
      type = req.url_params.get("type");
    }

    if (req.url_params.get("identifier") != nullptr) {
      identifier = req.url_params.get("identifier");
    }

    if (req.url_params.get("line") != nullptr) {
      line = std::stoi(req.url_params.get("line"));
    }

    if (req.url_params.get("column") != nullptr) {
      column = std::stoi(req.url_params.get("column"));
    }
    fmt::print(fmt::emphasis::bold | fg(fmt::color::green),
               "identifier {} line[{}] column[{}]\n", identifier, line, column);
    std::string content;
    std::string msg;
    json answer;
    if (type.compare("definition") == 0) {
      Definition definition(_lsp, root, identifier, line, column);
      definition.send();
      msg = definition.receive();
      fmt::print(fmt::emphasis::bold | fg(fmt::color::green),
                 "definition Received from clangd [{}]\n", msg);
    }
    if (type.compare("declaration") == 0) {
      Declaration declaration(_lsp, root, identifier, line, column);
      declaration.send();
      msg = declaration.receive();
      fmt::print(fmt::emphasis::bold | fg(fmt::color::green),
                 "declaration Received from clangd [{}]\n", msg);
    }
    if (type.compare("typeDefinition") == 0) {
      TypeDefinition typeDefinition(_lsp, root, identifier, line, column);
      typeDefinition.send();
      msg = typeDefinition.receive();
      fmt::print(fmt::emphasis::bold | fg(fmt::color::green),
                 "typeDefinition Received from clangd [{}]\n", msg);
    }
    if (type.compare("implementation") == 0) {
      Implementation implementation(_lsp, root, identifier, line, column);
      implementation.send();
      msg = implementation.receive();
      fmt::print(fmt::emphasis::bold | fg(fmt::color::green),
                 "implementation Received from clangd [{}]\n", msg);
    }
    // https://microsoft.github.io/language-server-protocol/specifications/lsp/3.17/specification/#textDocument_references
    if (type.compare("references") == 0) {
      References references(_lsp, root, identifier, line, column);
      references.send();
      msg = references.receive();
      fmt::print(fmt::emphasis::bold | fg(fmt::color::green),
                 "references Received from clangd [{}]\n", msg);
      answer = json::parse(msg);
      json responseArray = json::array();
      if (answer.contains("result")) {
        json resultArray = answer["result"];
        json entry;
        if (resultArray.is_array()) {
          for (json::iterator it = resultArray.begin(); it != resultArray.end();
               ++it) {
            entry = *it;
            if (entry.contains("uri")) {
              std::cout << "uri " << entry["uri"] << std::endl;
            }
            if (entry.contains("range")) {
              std::cout << "line " << entry["range"]["start"]["line"]
                        << std::endl;
            }
            std::string targetPath = entry["uri"];
            int targetLine = entry["range"]["start"]["line"];
            std::string tmpRoot = root.substr(0, root.size() - path.size());
            std::cout << "targetPath " << targetPath << std::endl;
            std::cout << "tmpRoot " << tmpRoot << std::endl;

            std::cout << "sub "
                      << targetPath.substr(std::string("file://").size() +
                                           tmpRoot.size())
                      << std::endl;
            std::string uri = _url + "/files?project=" + project + "&path=" +
                              targetPath.substr(std::string("file://").size() +
                                                tmpRoot.size()) +
                              "&line=" + std::to_string(targetLine + 1);
            std::cout << "uri " << uri << std::endl;
            std::string filename =
                targetPath.substr(targetPath.find_last_of("/\\") + 1) + ":" +
                std::to_string(targetLine + 1);
            responseArray.push_back({{"name", filename}, {"url", uri}});
          }
        }
      }
      json jsonResponse = {{"references", responseArray}};
      std::cout << "identifier json response " << jsonResponse.dump(4)
                << std::endl;
      auto res = crow::response{jsonResponse.dump()};
      res.add_header("Content-Type", "application/json");
      return res;
    }

    // https://github.com/MaskRay/ccls
    // https://github.com/jacobdufault/cquery

    if (type.compare("typeHierarchy") == 0) {
      TypeHierarchy typeHierarchy(_lsp, root, identifier, line, column);
      typeHierarchy.send();
      msg = typeHierarchy.receive();
      fmt::print(fmt::emphasis::bold | fg(fmt::color::green),
                 "typeHierarchy Received from clangd [{}]\n", msg);
      answer = json::parse(msg);
      json result;
      if (answer.contains("result")) {
        result = answer["result"];
      }
      json jsonResponse = {{"typeHierarchy", result}};
      std::cout << "typeHierarchy json response " << jsonResponse.dump(4)
                << std::endl;
      auto res = crow::response{jsonResponse.dump()};
      res.add_header("Content-Type", "application/json");
      return res;
    }

    answer = json::parse(msg);
    std::cout << "clang answer only json " << answer.dump(4) << std::endl;
    std::cout << "clang answer " << DefinitionAnswer::out(answer).dump(4)
              << std::endl;

    json firstResult;
    if (answer.contains("result")) {
      json resultArray = answer["result"];
      if (resultArray.is_array()) {
        for (json::iterator it = resultArray.begin(); it != resultArray.end();
             ++it) {
          firstResult = *it;
          if (firstResult.contains("uri")) {
            std::cout << "uri " << firstResult["uri"] << std::endl;
          }
          if (firstResult.contains("range")) {
            std::cout << "line " << firstResult["range"]["start"]["line"]
                      << std::endl;
          }
          break;
        }
      }
    }
    if (!firstResult.contains("uri")) {
      return crow::response{""};
    }
    std::string targetPath = firstResult["uri"];
    int targetLine = firstResult["range"]["start"]["line"];
    std::string tmpRoot = root.substr(0, root.size() - path.size());
    std::cout << "targetPath " << targetPath << std::endl;
    std::cout << "tmpRoot " << tmpRoot << std::endl;

    std::cout << "sub "
              << targetPath.substr(std::string("file://").size() +
                                   tmpRoot.size())
              << std::endl;
    std::string uri =
        _url + "/files?project=" + project + "&path=" +
        targetPath.substr(std::string("file://").size() + tmpRoot.size()) +
        "&line=" + std::to_string(targetLine + 1);
    std::cout << "uri " << uri << std::endl;
    json jsonResponse = {{"uri", uri}};
    //"http://localhost:3000/files?project=dhewm3&path=neo/framework/CVarSystem.h&line=118"
    std::cout << "identifier json response " << jsonResponse.dump(4)
              << std::endl;
    auto res = crow::response{jsonResponse.dump()};
    res.add_header("Content-Type", "application/json");
    return res;
  });

  // https://src.illumos.org/source/history/gfx-drm/myenv.sh
  // http://localhost:3000/diff?r1=neo/cm/CollisionModel.h#736ec20&r2=neo/cm/CollisionModel.h#79ad905&736ec20=on&79ad905=on
  CROW_ROUTE(app, "/diff")
  ([&](const crow::request& req) {
    ServerPrivData priv;
    fillVariables(_projects, req, priv);
    priv._filename = _htmlRoot + "/" + _diff;
    return crow::response{load(priv)};
  });

  // http://localhost:3000/annotate?project=dhewm3&path=neo/cm/CollisionModel.h
  CROW_ROUTE(app, "/annotate")
  ([&](const crow::request& req) {
    std::string body = DEFAULT_BODY;
    ServerPrivData priv;
    fillVariables(_projects, req, priv);
    priv._filename = _htmlRoot + "/" + _annotate;
    priv._useGit = true;
    return crow::response{load(priv)};
  });

  CROW_ROUTE(app, "/search")
  ([&](const crow::request& req) {
    // no project selected
    // http://localhost:3000/search?q=bogus&defs=eita&refs=symbol&path=path
    // one project selected
    // http://localhost:3000/search?q=bogus&defs=eita&refs=symbol&path=path&project=dhewm3
    // more project selected
    // http://localhost:3000/search?q=bogus&defs=eita&refs=symbol&path=path&project=dhewm3&project=dummy
    // https://libgit2.org/
    // https://github.com/libgit2/libgit2
    std::string body = DEFAULT_BODY;
    std::cout << "htmlRoot " << _htmlRoot << std::endl;
    std::cout << "req.url " << req.url << std::endl;
    ServerPrivData priv;
    fillVariables(_projects, req, priv);
    if (!priv._project.empty() && !priv._fullTextQuery.empty()) {
      std::cout << "query is [" << priv._fullTextQuery << "]" << std::endl;
      FullTextSearcher searcher(_projects[priv._project].second);
      auto results = searcher.search(priv._fullTextQuery);
      fs::path last, p;
      std::map<std::string, std::map<std::string, std::list<SearchLine>>> info;
      for (auto& entry : results) {
        fs::path p = entry.url;
        std::string filename = p.filename();
        p.remove_filename();
        std::string dir = p.string();
        info[dir][filename].push_back({entry.line, entry.excerpt});
        std::cout << entry.line << ":" << entry.url << ":" << entry.excerpt
                  << std::endl;
      }
      _searchInfo.clear();

      for (auto& dir : info) {
        std::cout << "dir " << dir.first << std::endl;
        SearchInfo info;
        info.path = dir.first;
        info.path.erase(0, 1);  // remove first /
        info.files.clear();
        for (auto& _file : dir.second) {
          SearchFile file;
          file.name = _file.first;
          file.lines.clear();
          std::cout << "   filename " << _file.first << std::endl;
          for (auto& _line : _file.second) {
            std::cout << "      " << _line.line << " " << _line.excerpt
                      << std::endl;
            file.lines.push_back({_line.line, _line.excerpt});
          }
          info.files.push_back(file);
        }
        _searchInfo.push_back(info);
      }
      priv._useSearch = true;
    }
    priv._useGit = true;
    priv._filename = _htmlRoot + "/" + _search;
    // std::cout << "page  " << load(priv) << std::endl;
    return crow::response{load(priv)};
  });

  // http://localhost:3000/header_source?project=dhewm3&path=neo/cm/CollisionModel.h
  CROW_ROUTE(app, "/header_source")
  ([&](const crow::request& req) {
    std::string body = DEFAULT_BODY;
    std::string project;
    std::string root;
    std::string path;
    if (req.url_params.get("project") != nullptr) {
      project = req.url_params.get("project");
      root = _projects[project].first;
      if (!root.empty()) {
        std::filesystem::current_path(fs::path(root));
      }
    }
    if (req.url_params.get("path") != nullptr) {
      path = req.url_params.get("path");
      root += '/' + path;
    }
    if (!exists(root)) {
      return crow::response{"{}"};
    }
    std::string msg;
    json answer;
    HeaderSource headerSource(_lsp, root);
    headerSource.send();
    msg = headerSource.receive();
    fmt::print(fmt::emphasis::bold | fg(fmt::color::green),
               "header source Received from clangd [{}]\n", msg);

    answer = json::parse(msg);
    std::string targetPath;
    if (!answer.empty()) {
      if (answer.contains("result")) {
        targetPath = answer["result"];
      }
    }
    std::string tmpRoot = root.substr(0, root.size() - path.size());
    std::cout << "targetPath " << targetPath << std::endl;
    std::cout << "tmpRoot " << tmpRoot << std::endl;

    std::cout << "sub "
              << targetPath.substr(std::string("file://").size() +
                                   tmpRoot.size())
              << std::endl;
    path = targetPath.substr(std::string("file://").size() + tmpRoot.size());
    std::string url =
        fmt::format("{}/files?project={}&path={}", _url, project, path);
    json headerSourceResponse = {{"url", url}};
    auto res = crow::response{headerSourceResponse.dump()};
    res.add_header("Content-Type", "application/json");
    return res;
  });
  Initialize initialize(_lsp);
  initialize.send();
  std::string msg = initialize.receive();
  fmt::print(fmt::emphasis::bold | fg(fmt::color::green),
             "Received from clangd [{}]\n", msg);

  app.port(_port).multithreaded().run();
}
