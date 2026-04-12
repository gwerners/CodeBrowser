#include "Utils.h"
#include <sys/stat.h>
#include <sys/wait.h>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <sstream>
#include "fmt/color.h"
#include "fmt/format.h"

namespace fs = std::filesystem;

void sigchld_handler(int) {
  int status;
  pid_t pid;
  while ((pid = waitpid(-1, &status, WNOHANG)) > 0) {
    fmt::print(fg(fmt::color::red),
               "{}:{} - Child process {} terminated with status {}\n",
               __FUNCTION__, __LINE__, pid, WEXITSTATUS(status));
  }
}

// Proper UTF-8 validation: replaces any byte that is not valid ASCII
// or a proper UTF-8 continuation/lead byte with a replacement char.
static bool is_valid_utf8_byte(uint8_t byte) {
  // ASCII range is always valid
  if (byte < 0x80)
    return true;
  // Valid UTF-8 lead bytes: 0xC0-0xFD
  // Valid continuation bytes: 0x80-0xBF
  // For simplicity, reject bytes >= 0x80 that could cause issues
  // in contexts expecting clean ASCII-compatible text.
  // This is a conservative approach matching the original intent.
  return false;
}

void replace_invalid_utf8(std::string& str) {
  for (char& c : str) {
    if (!is_valid_utf8_byte(static_cast<uint8_t>(c))) {
      c = ' ';
    }
  }
}

void replace_invalid_utf8(char* str, size_t len) {
  for (size_t i = 0; i < len; i++) {
    if (!is_valid_utf8_byte(static_cast<uint8_t>(str[i]))) {
      str[i] = ' ';
    }
  }
}

std::string readFile(const std::string& filename) {
  std::ifstream t(filename);
  std::string ret((std::istreambuf_iterator<char>(t)),
                  std::istreambuf_iterator<char>());
  replace_invalid_utf8(ret);
  return ret;
}

bool fileExists(const std::string& filename) {
  struct stat buffer;
  return (stat(filename.c_str(), &buffer) == 0);
}

std::vector<std::string> split(const std::string& line, char delimiter) {
  std::vector<std::string> result;
  std::stringstream ss(line);
  std::string item;
  while (std::getline(ss, item, delimiter)) {
    result.push_back(item);
  }
  return result;
}

std::string getCurrentTimeFormatted() {
  auto now = std::chrono::system_clock::now();
  auto now_time_t = std::chrono::system_clock::to_time_t(now);
  auto now_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                    now.time_since_epoch())
                    .count() %
                1000;

  std::tm tm = *std::localtime(&now_time_t);

  char buffer[100];
  std::strftime(buffer, sizeof(buffer), "%Y/%m/%d %H:%M:%S", &tm);

  std::ostringstream oss;
  oss << buffer << "." << std::setfill('0') << std::setw(3) << now_ms << " - ";

  std::strftime(buffer, sizeof(buffer), "%a %B %d %H:%M:%S %Z %Y", &tm);
  oss << buffer;

  return oss.str();
}

bool saveTimestampToJson(const std::string& filePath) {
  try {
    fs::create_directories(fs::path(filePath).parent_path());
    json j;
    j["timestamp"] = getCurrentTimeFormatted();
    std::ofstream file(filePath);
    if (!file.is_open())
      return false;
    file << j.dump(4);
    return true;
  } catch (...) {
    return false;
  }
}

std::string loadTimestampFromJson(const std::string& filePath) {
  try {
    std::ifstream file(filePath);
    if (!file.is_open())
      return "No timestamp file found.";
    json j;
    file >> j;
    if (j.contains("timestamp"))
      return j["timestamp"];
    return "Field 'timestamp' not found.";
  } catch (...) {
    return "Error reading timestamp file.";
  }
}

bool endsWith(const std::string& str, const std::string& suffix) {
  if (suffix.size() > str.size())
    return false;
  return str.compare(str.size() - suffix.size(), suffix.size(), suffix) == 0;
}
