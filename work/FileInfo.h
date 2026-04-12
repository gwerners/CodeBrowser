#pragma once
#include <strings.h>
#include <chrono>
#include <string>

class FileInfo {
 public:
  std::string path;
  std::string name;
  std::string access_date;
  uintmax_t size = 0;
  std::string type;  // "files" or "folders"
  bool isDir = false;

  bool operator<(const FileInfo& other) const {
    if (isDir != other.isDir)
      return isDir;  // directories first
    return strcasecmp(name.c_str(), other.name.c_str()) < 0;
  }
};

template <typename TP>
std::time_t to_time_t(TP tp) {
  using namespace std::chrono;
  auto sctp = time_point_cast<system_clock::duration>(
      tp - TP::clock::now() + system_clock::now());
  return system_clock::to_time_t(sctp);
}
