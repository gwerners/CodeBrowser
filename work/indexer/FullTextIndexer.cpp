#include "indexer/FullTextIndexer.h"
#include <dirent.h>
#include <string.h>
#include <filesystem>
#include "Utils.h"

#define CFISH_USE_SHORT_NAMES
#define LUCY_USE_SHORT_NAMES

#include "Clownfish/String.h"
#include "Lucy/Analysis/EasyAnalyzer.h"
#include "Lucy/Document/Doc.h"
#include "Lucy/Index/Indexer.h"
#include "Lucy/Plan/FullTextType.h"
#include "Lucy/Plan/Schema.h"
#include "Lucy/Plan/StringType.h"

static Schema* createSchema() {
  Schema* schema = Schema_new();
  String* language = Str_newf("en");
  EasyAnalyzer* analyzer = EasyAnalyzer_new(language);

  // Field: line number (stored, not indexed)
  {
    String* field = Str_newf("line");
    StringType* type = StringType_new();
    StringType_Set_Indexed(type, false);
    Schema_Spec_Field(schema, field, (FieldType*)type);
    DECREF(type);
    DECREF(field);
  }
  // Field: content (full-text searchable, highlightable)
  {
    String* field = Str_newf("content");
    FullTextType* type = FullTextType_new((Analyzer*)analyzer);
    FullTextType_Set_Highlightable(type, true);
    Schema_Spec_Field(schema, field, (FieldType*)type);
    DECREF(type);
    DECREF(field);
  }
  // Field: url/path (stored, not indexed)
  {
    String* field = Str_newf("url");
    StringType* type = StringType_new();
    StringType_Set_Indexed(type, false);
    Schema_Spec_Field(schema, field, (FieldType*)type);
    DECREF(type);
    DECREF(field);
  }

  DECREF(analyzer);
  DECREF(language);
  return schema;
}

static void indexFile(Indexer* indexer,
                      const char* root,
                      const char* filename) {
  FILE* stream = fopen(filename, "r");
  if (!stream)
    return;

  char* content = nullptr;
  size_t len = 0;
  unsigned int lineNum = 0;
  char lineStr[32];

  while (getline(&content, &len, stream) != -1) {
    ++lineNum;
    Doc* doc = Doc_new(nullptr, 0);

    // Store line number
    sprintf(lineStr, "%d", lineNum);
    {
      String* field = Str_newf("line");
      String* value = Str_new_from_utf8(lineStr, strlen(lineStr));
      Doc_Store(doc, field, (Obj*)value);
      DECREF(field);
      DECREF(value);
    }
    // Store content (sanitized)
    {
      replace_invalid_utf8(content, strlen(content));
      String* field = Str_newf("content");
      String* value = Str_new_from_utf8(content, strlen(content));
      Doc_Store(doc, field, (Obj*)value);
      DECREF(field);
      DECREF(value);
    }
    // Store URL (path relative to root)
    {
      size_t rootLen = strlen(root);
      String* field = Str_newf("url");
      String* value =
          Str_new_from_utf8(filename + rootLen, strlen(filename) - rootLen);
      Doc_Store(doc, field, (Obj*)value);
      DECREF(field);
      DECREF(value);
    }

    Indexer_Add_Doc(indexer, doc, 1.0);
    DECREF(doc);
  }

  free(content);
  fclose(stream);
}

static void indexDirectory(Indexer* indexer,
                           const char* root,
                           const char* dirPath,
                           const std::set<std::string>& extensions) {
  DIR* dir = opendir(dirPath);
  if (!dir)
    return;

  struct dirent* entry;
  while ((entry = readdir(dir)) != nullptr) {
    if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
      continue;

    char path[1024];
    snprintf(path, sizeof(path), "%s/%s", dirPath, entry->d_name);

    if (entry->d_type == DT_DIR) {
      indexDirectory(indexer, root, path, extensions);
    } else {
      // Check extension
      const char* dot = strrchr(entry->d_name, '.');
      if (dot) {
        std::string ext(dot + 1);
        if (extensions.empty() || extensions.count(ext)) {
          indexFile(indexer, root, path);
        }
      }
    }
  }
  closedir(dir);
}

// Default extensions to index
static const std::set<std::string> DEFAULT_EXTENSIONS = {
    "c",   "cpp", "h",  "hpp", "cxx", "hxx", "cc",
    "js",  "py",  "go", "rs",  "java"};

FullTextIndexer::FullTextIndexer(const std::string& indexPath,
                                 const std::string& sourceDir,
                                 const std::set<std::string>& extensions)
    : _indexPath(indexPath),
      _sourceDir(sourceDir),
      _extensions(extensions.empty() ? DEFAULT_EXTENSIONS : extensions) {}

void FullTextIndexer::update() {
  // Create index directory if it doesn't exist
  std::filesystem::create_directories(_indexPath);

  lucy_bootstrap_parcel();

  Schema* schema = createSchema();
  String* folder = Str_newf("%s", _indexPath.c_str());
  Indexer* indexer =
      Indexer_new(schema, (Obj*)folder, nullptr, Indexer_CREATE | Indexer_TRUNCATE);

  indexDirectory(indexer, _sourceDir.c_str(), _sourceDir.c_str(), _extensions);

  Indexer_Commit(indexer);

  DECREF(indexer);
  DECREF(folder);
  DECREF(schema);
}
