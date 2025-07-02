#include "FullTextSearcher.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "nlohmann/json.hpp"

#define CFISH_USE_SHORT_NAMES
#define LUCY_USE_SHORT_NAMES
#include "Clownfish/String.h"
#include "Clownfish/Vector.h"
#include "Lucy/Document/HitDoc.h"
#include "Lucy/Highlight/Highlighter.h"
#include "Lucy/Plan/Schema.h"
#include "Lucy/Search/ANDQuery.h"
#include "Lucy/Search/Hits.h"
#include "Lucy/Search/IndexSearcher.h"
#include "Lucy/Search/QueryParser.h"
#include "Lucy/Search/TermQuery.h"

using json = nlohmann::json;

FullTextSearcher::FullTextSearcher(const std::string& path) : _indexPath(path) {
  // Initialize the library.
  lucy_bootstrap_parcel();
}

std::vector<SearchResult> FullTextSearcher::search(const std::string& needle) {
  std::vector<SearchResult> result;
  const char* query_c = needle.c_str();

  // printf("Searching for: %s\n\n", query_c);

  String* folder = Str_newf("%s", _indexPath.c_str());
  IndexSearcher* searcher = IxSearcher_new((Obj*)folder);
  Schema* schema = IxSearcher_Get_Schema(searcher);
  QueryParser* qparser = QParser_new(schema, NULL, NULL, NULL);

  String* query_str = Str_newf("%s", query_c);
  Query* query = QParser_Parse(qparser, query_str);

  String* content_str = Str_newf("content");
  Highlighter* highlighter =
      Highlighter_new((Searcher*)searcher, (Obj*)query, content_str, 200);

  Hits* hits = IxSearcher_Hits(searcher, (Obj*)query, 0, 50000, NULL);

  String* line_str = Str_newf("line");
  String* url_str = Str_newf("url");
  HitDoc* hit;
  int i = 1;
  // Loop over search results.
  while (NULL != (hit = Hits_Next(hits))) {
    String* line = (String*)HitDoc_Extract(hit, line_str);
    char* line_c = Str_To_Utf8(line);

    String* url = (String*)HitDoc_Extract(hit, url_str);
    char* url_c = Str_To_Utf8(url);

    String* excerpt = Highlighter_Create_Excerpt(highlighter, hit);
    char* excerpt_c = Str_To_Utf8(excerpt);

    // printf("Result %d: %s (%s)\n%s\n\n", i, line_c, url_c, excerpt_c);
    // json ret = {{"line", line_c}, {"url", url_c}, {"excerpt", excerpt_c}};
    // printf("%d %s\n", i, ret.dump(4).c_str());
    SearchResult sr(line_c, url_c, excerpt_c);
    result.push_back(sr);

    free(excerpt_c);
    free(url_c);
    free(line_c);
    DECREF(excerpt);
    DECREF(url);
    DECREF(line);
    DECREF(hit);
    i++;
  }
  std::sort(result.begin(), result.end());

  DECREF(url_str);
  DECREF(line_str);
  DECREF(hits);
  DECREF(query);
  DECREF(query_str);
  DECREF(highlighter);
  DECREF(content_str);
  DECREF(qparser);
  DECREF(searcher);
  DECREF(folder);
  return result;
}

SearchResult::SearchResult(std::string _line,
                           std::string _url,
                           std::string _excerpt)
    : line(_line), url(_url), excerpt(_excerpt) {
  excerpt.erase(std::remove(excerpt.begin(), excerpt.end(), '\n'),
                excerpt.end());
  excerpt.erase(std::remove(excerpt.begin(), excerpt.end(), '\\'),
                excerpt.end());
}
