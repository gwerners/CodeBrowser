#include "Utils.h"
#include <dirent.h>
#include <signal.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <fstream>
#include <nlohmann/json.hpp>
#include <sstream>
#include <string>
#include <vector>
#include "fmt/color.h"
#include "fmt/format.h"

namespace fs = std::filesystem;
using json = nlohmann::json;

void sigchld_handler(int) {
  int status;
  pid_t pid;

  // Reap all terminated child processes
  while ((pid = waitpid(-1, &status, WNOHANG)) > 0) {
    // Handle terminated child process
    fmt::print(fg(fmt::color::blue),
               "{}:{} - Child process {} terminated with status {}\n",
               __FUNCTION__, __LINE__, pid, WEXITSTATUS(status));
  }
}

bool is_valid_utf8_char(uint8_t byte) {
  // ASCII characters are always valid
  if (byte < 0x80)
    return true;

  // Check if byte is part of a multi-byte sequence
  if ((byte & 0xC0) != 0x80)
    return false;

  // 0xA9  invalid UTF-8 byte at index 1792: 0xA9
  if (byte & 0xA9)
    return false;

  if (byte > 127)
    return false;
  return true;
}

void replace_invalid_utf8(std::string& str) {
  for (char& c : str) {
    if (!is_valid_utf8_char(static_cast<uint8_t>(c))) {
      c = '#';
    }
  }
}

std::string readFile(const std::string& filename) {
  std::ifstream t(filename);
  std::string ret = std::string((std::istreambuf_iterator<char>(t)),
                                std::istreambuf_iterator<char>());
  replace_invalid_utf8(ret);
  /*fmt::print( fg(fmt::color::blue),
             "{}:{} - {} contents[{}]\n",__FUNCTION__,__LINE__,filename,ret);*/
  return ret;
}

bool exists(const std::string& filename) {
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

  // Format: 2025/5/3 15:00:00.050 - Thu July 03 15:00:00 PDT 2025
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

    file << j.dump(4);  // indentado
    file.close();

    return true;
  } catch (...) {
    return false;
  }
}
std::string loadTimestampFromJson(const std::string& filePath) {
  try {
    std::ifstream file(filePath);
    if (!file.is_open())
      return "Erro ao abrir arquivo.";

    json j;
    file >> j;
    file.close();

    if (j.contains("timestamp")) {
      return j["timestamp"];
    }

    return "Campo 'timestamp' não encontrado.";
  } catch (...) {
    return "Erro ao ler o arquivo ou JSON inválido.";
  }
}
