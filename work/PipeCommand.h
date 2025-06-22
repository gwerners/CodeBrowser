#pragma once
#include <unistd.h>
#include <string>
// #include <iostream>
#include <string.h>

enum class PipeError { Success = 0, Pipe, Fork, Execlp, Read };

/*template <typename Arg, typename... Args>
void debugPrint(std::ostream& out, Arg&& arg, Args&&... args)
{
    out << std::forward<Arg>(arg);
    using expander = int[];
    (void)expander{0, (void(out << ' ' << std::forward<Args>(args)), 0)...};
}*/

class PipeCommand {
 public:
  template <typename... Args>
  static std::string cmd(const std::string& cmd, Args... args) {
    int _toChild[2];
    int _fromChild[2];
    // debug
    /*std::cout << "cmd " << cmd << " ";
    debugPrint(std::cout,args...);
    std::cout << std::endl;*/

    // Create pipes
    if (pipe(_toChild) == -1 || pipe(_fromChild) == -1) {
      return "";
    }
    pid_t pid = fork();
    if (pid == -1) {
      return "";
    } else if (pid == 0) {   // Child process (clangd)
      close(_toChild[1]);    // Close write end of the pipe to clangd
      close(_fromChild[0]);  // Close read end of the pipe from clangd
      // Redirect pipes to stdin and stdout for clangd
      dup2(_toChild[0], STDIN_FILENO);
      dup2(_fromChild[1], STDOUT_FILENO);
      // Close unused pipe ends
      close(_toChild[0]);
      close(_fromChild[1]);
      // Execute command
      execlp(cmd.c_str(), cmd.c_str(), args..., nullptr);
      return "";
    } else {
      // Parent process (controller)
      close(_toChild[0]);
      close(_fromChild[1]);
      std::string ret;
      int pipe = _fromChild[0];
      char buffer[2048] = {0};
      ssize_t bytes_read;

      do {
        memset(buffer, 0, sizeof(buffer));
        bytes_read = read(pipe, buffer, sizeof(buffer) - 1);
        if (bytes_read == -1) {
          return "";
        } else {
          ret.append(buffer);
        }
        //} while (bytes_read == sizeof(buffer) - 1);
      } while (bytes_read > 0);
      close(_toChild[1]);
      close(_fromChild[0]);
      return ret;
    }
  }
};
