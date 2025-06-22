#include "Utils.h"
#include <dirent.h>
#include <signal.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include "fmt/color.h"
#include "fmt/format.h"

namespace fs = std::filesystem;

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
