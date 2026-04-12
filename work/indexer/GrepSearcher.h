#pragma once
#include <string>
#include <vector>

// Search result compatible with FullTextSearcher's SearchResult
struct GrepResult {
  std::string line;     // line number as string
  std::string url;      // relative path within project
  std::string excerpt;  // raw line content (no highlighting)

  bool operator<(const GrepResult& other) const { return url < other.url; }
};

// Full-text search using ripgrep (preferred) or grep (fallback).
// Replacement for Apache Lucy when full-text indexing is not available.
// No index needed - searches source files directly.
class GrepSearcher {
 public:
  GrepSearcher(const std::string& sourceDir);

  std::vector<GrepResult> search(const std::string& query,
                                 const std::string& regexp = "");

 private:
  std::string _sourceDir;
  bool _useRipgrep;

  static bool hasRipgrep();
};
