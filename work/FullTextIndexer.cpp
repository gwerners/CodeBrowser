#include "FullTextIndexer.h"
#include <dirent.h>
#include <string.h>

#define CFISH_USE_SHORT_NAMES
#define LUCY_USE_SHORT_NAMES

#include "Clownfish/String.h"
#include "Lucy/Analysis/EasyAnalyzer.h"
#include "Lucy/Document/Doc.h"
#include "Lucy/Index/Indexer.h"
#include "Lucy/Plan/FullTextType.h"
#include "Lucy/Plan/Schema.h"
#include "Lucy/Plan/StringType.h"

// https://lucy.apache.org/docs/c/Lucy/Docs/Tutorial/BeyondSimpleTutorial.html
static Schema* S_create_schema() {
  // Create a new schema.
  Schema* schema = Schema_new();

  // Create an analyzer.
  String* language = Str_newf("en");
  EasyAnalyzer* analyzer = EasyAnalyzer_new(language);

  // Specify fields.
  {
    String* field_str = Str_newf("line");
    StringType* type = StringType_new();
    StringType_Set_Indexed(type, false);
    Schema_Spec_Field(schema, field_str, (FieldType*)type);
    DECREF(type);
    DECREF(field_str);
  }
  {
    String* field_str = Str_newf("content");
    FullTextType* type = FullTextType_new((Analyzer*)analyzer);
    FullTextType_Set_Highlightable(type, true);
    Schema_Spec_Field(schema, field_str, (FieldType*)type);
    DECREF(type);
    DECREF(field_str);
  }
  {
    String* field_str = Str_newf("url");
    StringType* type = StringType_new();
    StringType_Set_Indexed(type, false);
    Schema_Spec_Field(schema, field_str, (FieldType*)type);
    DECREF(type);
    DECREF(field_str);
  }

  DECREF(analyzer);
  DECREF(language);
  return schema;
}

bool S_starts_with(const char* str, const char* prefix) {
  size_t len = strlen(str);
  size_t prefix_len = strlen(prefix);

  return len >= prefix_len && memcmp(str, prefix, prefix_len) == 0;
}

bool S_ends_with(const char* str, const char* postfix) {
  size_t len = strlen(str);
  size_t postfix_len = strlen(postfix);

  return len >= postfix_len &&
         memcmp(str + len - postfix_len, postfix, postfix_len) == 0;
}

static bool is_valid_utf8_char(uint8_t byte) {
  // ASCII characters are always valid
  if (byte < 0x80)
    return true;

  // Check if byte is part of a multi-byte sequence
  if ((byte & 0xC0) != 0x80)
    return false;

  if (byte > 127)
    return false;
  return true;
}

static void replace_invalid_utf8(char* str) {
  unsigned int len = strlen(str);
  for (unsigned int i = 0; i < len; i++) {
    if (!is_valid_utf8_char(str[i])) {
      str[i] = ' ';
    }
  }
}

void S_parse_file(Indexer* indexer, const char* root, const char* filename) {
  size_t bytes = strlen(filename) + 1;
  char* path = (char*)malloc(bytes);
  path[0] = '\0';
  strcat(path, filename);

  FILE* stream = fopen(path, "r");
  if (stream == NULL) {
    perror(path);
    exit(1);
  }
  char* content = NULL;
  size_t len = 0;
  ssize_t read;
  unsigned int line = 0;
  char* lineStr = (char*)malloc(50 + 1);

  while ((read = getline(&content, &len, stream)) != -1) {
    ++line;
    Doc* doc = Doc_new(NULL, 0);
    {
      sprintf(lineStr, "%d", line);
      // Store 'line' field
      String* field = Str_newf("line");
      String* value = Str_new_from_utf8(lineStr, strlen(lineStr));
      Doc_Store(doc, field, (Obj*)value);
      DECREF(field);
      DECREF(value);
    }
    {
      // Store 'content' field
      replace_invalid_utf8(content);
      String* field = Str_newf("content");
      String* value = Str_new_from_utf8(content, strlen(content));
      Doc_Store(doc, field, (Obj*)value);
      DECREF(field);
      DECREF(value);
    }
    {
      // Store 'url' field
      int len = strlen(root);
      String* field = Str_newf("url");
      String* value = Str_new_from_utf8(filename + len, strlen(filename) - len);
      Doc_Store(doc, field, (Obj*)value);
      DECREF(field);
      DECREF(value);
    }

    Indexer_Add_Doc(indexer, doc, 1.0);
    DECREF(doc);
  }

  if (content)
    free(content);

  fclose(stream);
  free(path);
  free(lineStr);
  return;
}

void handleDir(Indexer* indexer, const char* root, const char* name) {
  DIR* dir;
  struct dirent* entry;

  if (!(dir = opendir(name)))
    return;
  // printf("DIR %s\n", name);
  while ((entry = readdir(dir)) != NULL) {
    char path[1024];
    snprintf(path, sizeof(path), "%s/%s", name, entry->d_name);
    if (entry->d_type == DT_DIR) {
      if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
        continue;
      // printf("NEXT DIR %s\n", path);
      handleDir(indexer, root, path);
    } else {
      if (S_ends_with(entry->d_name, ".c") ||
          S_ends_with(entry->d_name, ".cpp") ||
          S_ends_with(entry->d_name, ".h") ||
          S_ends_with(entry->d_name, ".hpp") ||
          S_ends_with(entry->d_name, ".cxx") ||
          S_ends_with(entry->d_name, ".hxx") ||
          S_ends_with(entry->d_name, ".cc") ||
          S_ends_with(entry->d_name, ".cc") ||
          S_ends_with(entry->d_name, ".js")) {
        // printf("+%s\n", path);
        S_parse_file(indexer, root, (char*)path);
      } else {
        // printf("-%s\n", entry->d_name);
      }
    }
  }
  closedir(dir);
}

FullTextIndexer::FullTextIndexer(const std::string& path,
                                 const std::string& source)
    : _indexPath(path), _indexSource(source) {}

void FullTextIndexer::update() {
  // Initialize the library.
  lucy_bootstrap_parcel();

  Schema* schema = S_create_schema();
  String* folder = Str_newf("%s", _indexPath.c_str());

  Indexer* indexer = Indexer_new(schema, (Obj*)folder, NULL,
                                 Indexer_CREATE | Indexer_TRUNCATE);
  handleDir(indexer, _indexSource.c_str(), _indexSource.c_str());

  Indexer_Commit(indexer);

  DECREF(indexer);
  DECREF(folder);
  DECREF(schema);
}
