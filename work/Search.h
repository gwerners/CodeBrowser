#pragma once
#include <string>
#include <vector>

class SearchLine {
 public:
  std::string line;
  std::string excerpt;
};

class SearchFile {
 public:
  std::string name;
  std::vector<SearchLine> lines;
};

class SearchInfo {
 public:
  std::string path;
  std::vector<SearchFile> files;
};
