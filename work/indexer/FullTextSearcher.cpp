#include "indexer/FullTextSearcher.h"
#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <cstring>

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

#include "re.h"

SearchResult::SearchResult(std::string l, std::string u, std::string e)
    : line(std::move(l)), url(std::move(u)), excerpt(std::move(e)) {
  // Clean up excerpt
  excerpt.erase(std::remove(excerpt.begin(), excerpt.end(), '\n'),
                excerpt.end());
  excerpt.erase(std::remove(excerpt.begin(), excerpt.end(), '\\'),
                excerpt.end());
}

FullTextSearcher::FullTextSearcher(const std::string& indexPath)
    : _indexPath(indexPath) {
  lucy_bootstrap_parcel();
}

std::vector<SearchResult> FullTextSearcher::search(
    const std::string& needle,
    const std::string& regexp) {
  std::vector<SearchResult> results;

  re_t regexPattern;
  bool useRegexp = !regexp.empty();
  if (useRegexp)
    regexPattern = re_compile(regexp.c_str());

  String* folder = Str_newf("%s", _indexPath.c_str());
  IndexSearcher* searcher = IxSearcher_new((Obj*)folder);
  Schema* schema = IxSearcher_Get_Schema(searcher);
  QueryParser* qparser = QParser_new(schema, nullptr, nullptr, nullptr);

  String* queryStr = Str_newf("%s", needle.c_str());
  Query* query = QParser_Parse(qparser, queryStr);

  String* contentField = Str_newf("content");
  Highlighter* highlighter =
      Highlighter_new((Searcher*)searcher, (Obj*)query, contentField, 200);

  Hits* hits = IxSearcher_Hits(searcher, (Obj*)query, 0, 50000, nullptr);

  String* lineField = Str_newf("line");
  String* urlField = Str_newf("url");

  HitDoc* hit;
  while ((hit = Hits_Next(hits)) != nullptr) {
    String* lineStr = (String*)HitDoc_Extract(hit, lineField);
    String* urlStr = (String*)HitDoc_Extract(hit, urlField);
    String* excerptStr = (String*)HitDoc_Extract(hit, contentField);
    String* highlighted = Highlighter_Create_Excerpt(highlighter, hit);

    char* line_c = Str_To_Utf8(lineStr);
    char* url_c = Str_To_Utf8(urlStr);
    char* excerpt_c = Str_To_Utf8(excerptStr);
    char* highlighted_c = Str_To_Utf8(highlighted);

    // Apply regex filter if specified
    bool match = true;
    if (useRegexp) {
      int len;
      match = (re_matchp(regexPattern, excerpt_c, &len) != -1);
    }

    if (match)
      results.emplace_back(line_c, url_c, highlighted_c);

    free(highlighted_c);
    free(excerpt_c);
    free(url_c);
    free(line_c);
    DECREF(highlighted);
    DECREF(excerptStr);
    DECREF(urlStr);
    DECREF(lineStr);
    DECREF(hit);
  }

  std::sort(results.begin(), results.end());

  DECREF(urlField);
  DECREF(lineField);
  DECREF(hits);
  DECREF(query);
  DECREF(queryStr);
  DECREF(highlighter);
  DECREF(contentField);
  DECREF(qparser);
  DECREF(searcher);
  DECREF(folder);

  return results;
}
