#pragma once
#include "SearchFile.h"

class SearchInfo {
 public:
  std::string path;
  std::vector<SearchFile> files;
};
