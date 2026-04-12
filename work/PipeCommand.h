#pragma once
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>
#include <string>

class PipeCommand {
 public:
  template <typename... Args>
  static std::string cmd(const std::string& cmd, Args... args) {
    int toChild[2];
    int fromChild[2];

    if (pipe(toChild) == -1 || pipe(fromChild) == -1)
      return "";

    pid_t pid = fork();
    if (pid == -1)
      return "";

    if (pid == 0) {
      // Child process
      close(toChild[1]);
      close(fromChild[0]);
      dup2(toChild[0], STDIN_FILENO);
      dup2(fromChild[1], STDOUT_FILENO);
      close(toChild[0]);
      close(fromChild[1]);
      execlp(cmd.c_str(), cmd.c_str(), args..., nullptr);
      _exit(1);
    }

    // Parent process
    close(toChild[0]);
    close(fromChild[1]);

    // Read all output before waiting for child
    std::string ret;
    char buffer[4096] = {0};
    ssize_t bytes_read;
    do {
      memset(buffer, 0, sizeof(buffer));
      bytes_read = read(fromChild[0], buffer, sizeof(buffer) - 1);
      if (bytes_read > 0)
        ret.append(buffer, bytes_read);
    } while (bytes_read > 0);

    close(toChild[1]);
    close(fromChild[0]);

    // Reap child process ourselves so sigchld_handler doesn't interfere
    waitpid(pid, nullptr, 0);

    return ret;
  }
};
