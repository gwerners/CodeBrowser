#include "Git.h"
#include "PipeCommand.h"
#include "Utils.h"
#include "fmt/color.h"
#include "fmt/format.h"
#include "nlohmann/json.hpp"

using json = nlohmann::json;

static json parseGitLog(const std::string& input) {
  json jsonArray = json::array();
  if (input.empty())
    return jsonArray;

  std::istringstream stream(input);
  std::string line;

  while (std::getline(stream, line)) {
    if (line.empty())
      continue;

    auto parts = split(line, '|');
    if (parts.size() != 4)
      continue;

    // Remove quotes from git pretty format
    for (auto& p : parts) {
      p.erase(std::remove(p.begin(), p.end(), '"'), p.end());
    }

    json entry;
    entry["hash"] = parts[0];
    entry["date"] = parts[1];
    entry["author"] = parts[2];
    entry["message"] = parts[3];

    std::vector<std::string> files;
    while (std::getline(stream, line) && !line.empty()) {
      files.push_back(line);
    }
    entry["files"] = files;
    jsonArray.push_back(entry);
  }
  return jsonArray;
}

// Find git repo root from current directory
static std::string findGitRoot() {
  std::string root = PipeCommand::cmd("git", "rev-parse", "--show-toplevel");
  while (!root.empty() && (root.back() == '\n' || root.back() == '\r'))
    root.pop_back();
  return root;
}

// Convert path (relative to cwd) to path relative to git root
static std::string toGitPath(const std::string& path,
                             const std::string& gitRoot) {
  if (gitRoot.empty())
    return path;

  char cwd[4096];
  if (!getcwd(cwd, sizeof(cwd)))
    return path;

  std::string absPath = std::string(cwd) + "/" + path;
  if (absPath.find(gitRoot) == 0) {
    std::string rel = absPath.substr(gitRoot.size());
    if (!rel.empty() && rel[0] == '/')
      rel = rel.substr(1);
    return rel;
  }
  return path;
}

// Convert path (relative to git root) to path relative to cwd
static std::string fromGitPath(const std::string& gitPath,
                               const std::string& gitRoot) {
  if (gitRoot.empty())
    return gitPath;

  char cwd[4096];
  if (!getcwd(cwd, sizeof(cwd)))
    return gitPath;

  // cwd prefix relative to gitRoot (e.g. "compiler/")
  std::string cwdStr(cwd);
  if (cwdStr.find(gitRoot) == 0) {
    std::string prefix = cwdStr.substr(gitRoot.size());
    if (!prefix.empty() && prefix[0] == '/')
      prefix = prefix.substr(1);
    if (!prefix.empty() && prefix.back() != '/')
      prefix += '/';

    // Strip prefix from gitPath if it matches
    if (!prefix.empty() && gitPath.find(prefix) == 0)
      return gitPath.substr(prefix.size());
  }
  return gitPath;
}

Git::Git(const std::string& gitCmd) : _git(gitCmd) {
  _gitRoot = findGitRoot();
}

std::string Git::blame(const std::string& path, const std::string& hash) {
  std::string gp = toGitPath(path, _gitRoot);
  if (hash.empty())
    return PipeCommand::cmd(_git, "-C", _gitRoot.c_str(),
                            "blame", "--", gp.c_str());
  return PipeCommand::cmd(_git, "-C", _gitRoot.c_str(),
                          "blame", hash.c_str(), "--", gp.c_str());
}

std::string Git::log(const std::string& path) {
  std::string gp = toGitPath(path, _gitRoot);
  return PipeCommand::cmd(_git, "-C", _gitRoot.c_str(),
                          "log", "--pretty=format:\"%h|%ad|%an|%s\"",
                          "--date=short", "--name-only", "--", gp.c_str());
}

std::string Git::show(const std::string& path, const std::string& hash) {
  std::string gp = toGitPath(path, _gitRoot);
  std::string ref = fmt::format("{}:{}", hash, gp);
  return PipeCommand::cmd(_git, "-C", _gitRoot.c_str(),
                          "show", ref.c_str());
}

std::vector<Commit> Git::commits(const std::string& path) {
  std::vector<Commit> result;
  std::string output = log(path);
  if (output.empty()) {
    fmt::print(fg(fmt::color::red), "Empty git log for {}\n", path);
    return result;
  }

  json entries = parseGitLog(output);
  for (auto& entry : entries) {
    Commit c;
    c.hash = entry.value("hash", "");
    c.date = entry.value("date", "");
    c.author = entry.value("author", "");
    c.message = entry.value("message", "");
    if (entry.contains("files")) {
      auto rawFiles = entry["files"].get<std::vector<std::string>>();
      for (auto& f : rawFiles)
        c.files.push_back(fromGitPath(f, _gitRoot));
    }
    result.push_back(std::move(c));
  }
  return result;
}
