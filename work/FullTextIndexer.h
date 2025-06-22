#pragma once
#include <string>

class FullTextIndexer {
 public:
  FullTextIndexer(const std::string& path, const std::string& source);
  void update();

 private:
  std::string _indexPath;
  std::string _indexSource;
};
