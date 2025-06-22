#pragma once
#include <string>
#include <vector>

struct Commit {
  std::string hash;
  std::string date;
  std::string author;
  std::string message;
  std::vector<std::string> files;
};

class Git {
 public:
  Git(const std::string& cmd);
  std::string blame(const std::string& path, const std::string& hash);
  std::string log(const std::string path);
  std::string show(const std::string& path, const std::string& hash);
  std::vector<Commit> commits(const std::string path);

 private:
  std::string _git;
};
