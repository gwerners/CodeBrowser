#pragma once
#include <vector>
#include "SearchLine.h"

class SearchFile {
 public:
  std::string name;
  std::vector<SearchLine> lines;
};
