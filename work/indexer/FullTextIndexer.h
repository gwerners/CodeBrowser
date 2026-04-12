#pragma once
#include <set>
#include <string>

class FullTextIndexer {
 public:
  FullTextIndexer(const std::string& indexPath,
                  const std::string& sourceDir,
                  const std::set<std::string>& extensions = {});
  void update();

 private:
  std::string _indexPath;
  std::string _sourceDir;
  std::set<std::string> _extensions;
};
