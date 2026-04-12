#pragma once
#include <string>
#include <vector>

struct SearchResult {
  std::string line;
  std::string url;
  std::string excerpt;

  SearchResult(std::string l, std::string u, std::string e);
  bool operator<(const SearchResult& other) const {
    return url < other.url;
  }
};

class FullTextSearcher {
 public:
  FullTextSearcher(const std::string& indexPath);
  std::vector<SearchResult> search(const std::string& query,
                                   const std::string& regexp = "");

 private:
  std::string _indexPath;
};
