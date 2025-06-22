#include "Git.h"
#include "PipeCommand.h"
#include "fmt/color.h"
#include "fmt/format.h"
#include "nlohmann/json.hpp"

using json = nlohmann::json;

// Function to parse a line based on a delimiter
std::vector<std::string> splitLine(const std::string& line, char delimiter) {
  std::vector<std::string> result;
  std::stringstream ss(line);
  std::string item;
  while (std::getline(ss, item, delimiter)) {
    item.erase(remove(item.begin(), item.end(), '"'), item.end());
    result.push_back(item);
  }
  return result;
}

// Function to parse the input and construct the JSON object
static json parseInput(const std::string& input) {
  json jsonArray = json::array();
  std::istringstream inputStream(input);
  std::string line;
  if (input.empty()) {
    fmt::print(fg(fmt::color::blue), "empty input in parseInput\n");
    return jsonArray;
  }
  while (std::getline(inputStream, line)) {
    // Skip empty lines
    if (line.empty())
      continue;

    std::vector<std::string> parsedData = splitLine(line, '|');

    // Skip invalid lines
    if (parsedData.size() != 4)
      continue;

    json jsonObject;
    jsonObject["hash"] = parsedData[0];
    jsonObject["date"] = parsedData[1];
    jsonObject["author"] = parsedData[2];
    jsonObject["message"] = parsedData[3];

    // Collect all related files
    std::vector<std::string> relatedFiles;
    while (std::getline(inputStream, line) && !line.empty()) {
      relatedFiles.push_back(line);
    }

    jsonObject["files"] = relatedFiles;
    jsonArray.push_back(jsonObject);
  }

  return jsonArray;
}

Git::Git(const std::string& cmd) : _git(cmd) {}

std::string Git::blame(const std::string& path, const std::string& hash) {
  if (hash.empty()) {
    return PipeCommand::cmd(_git, "blame", "--", path.c_str());
  }
  return PipeCommand::cmd(_git, "blame", hash.c_str(), "--", path.c_str());
}

std::string Git::log(const std::string path) {
  // clang-format off
    /*
    1 file
    git log --pretty=format:"%h|%ad|%an|%s" --date=short -- CollisionModel.h
    736ec20|2011-12-16|dhewg|Untangle the epic precompiled.h mess
    79ad905|2011-12-06|dhewg|Fix all whitespace errors
    ff493f6|2011-12-06|dhewg|Fix quoting in GPL headers
    fb1609f|2011-11-22|Timothee 'TTimo' Besset|hello world

    directory
    git log --pretty=format:"%h|%ad|%an|%s" --date=short --name-only -- $(pwd)
    dbe4174|2023-01-05|Daniel Gibson|Fix/work around other misc. compiler warnings
    neo/cm/CollisionModel_load.cpp
    */
  // clang-format on
  return PipeCommand::cmd(_git, "log", "--pretty=format:\"%h|%ad|%an|%s\"",
                          "--date=short", "--name-only", "--", path.c_str());
}

std::string Git::show(const std::string& path, const std::string& hash) {
  // git blame {commit_id} -- {path/to/file} --date=format:'%Y-%m-%d %H:%M:%S'
  std::string cmd = fmt::format("{}:{}", hash.c_str(), path.c_str());
  return PipeCommand::cmd(_git, "show", cmd.c_str());
}

std::vector<Commit> Git::commits(const std::string path) {
  std::vector<Commit> commits;
  std::string gitOutput = log(path);
  if (!gitOutput.empty()) {
    json gitJson = parseInput(gitOutput);
    fmt::print(fmt::emphasis::bold | fg(fmt::color::blue), "git json [{}]\n",
               gitJson.dump(4));
    if (!gitJson.empty()) {
      for (size_t index = 0; index < gitJson.size(); ++index) {
        json entry = gitJson.at(index);
        Commit commit;
        if (entry.contains("hash")) {
          commit.hash = entry["hash"];
        }
        if (entry.contains("date")) {
          commit.date = entry["date"];
        }
        if (entry.contains("author")) {
          commit.author = entry["author"];
        }
        if (entry.contains("message")) {
          commit.message = entry["message"];
        }
        if (entry.contains("files")) {
          commit.files = entry["files"];
        }
        commits.push_back(commit);
      }
    }
  } else {
    fmt::print(fmt::emphasis::bold | fg(fmt::color::red), "empty git!!!\n");
  }
  return commits;
}
