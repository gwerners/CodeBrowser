#pragma once
#include <string>
#include <vector>

struct SearchLine {
  std::string line;
  std::string excerpt;
};

struct SearchFile {
  std::string name;
  std::vector<SearchLine> lines;
};

struct SearchInfo {
  std::string path;
  std::vector<SearchFile> files;
};
