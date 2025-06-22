#include "PipeAdapter.h"
#include <fcntl.h>
#include <fmt/color.h>
#include <fmt/format.h>
#include <poll.h>
#include <string.h>
#include <sys/select.h>

PipeAdapter* _instance = nullptr;
bool g_stop = false;
int g_tmpFile = -1;
int g_to;
int g_from;
int g_id;
int g_wakeFile = -1;
std::mutex g_pipeMutex;

std::list<std::pair<int, std::string>> g_send;
std::list<std::pair<int, std::string>> g_receive;

bool PipeAdapter::makeTempFile() {
  const char* tmpdir = nullptr;
  // tmpdir = getenv("TMPDIR");
  if (tmpdir == nullptr) {
    tmpdir = "/home/gwerners/projects/CodeBrowser/build";
  }
  if (asprintf(&_tmpFilename, "%s/clangPipe-tmp-XXXXXX", tmpdir) < 0) {
    fmt::print(fmt::emphasis::bold | fg(fmt::color::red),
               "unable to create tmpFile name for thread\n");
    return -ENOMEM;
  }
  g_tmpFile = mkostemp(_tmpFilename, O_CREAT | O_RDWR);
  g_wakeFile = open(_tmpFilename, O_RDWR);
  if (g_tmpFile < 0) {
    fmt::print(fmt::emphasis::bold | fg(fmt::color::red),
               "unable to create tmpFile\n");

    return false;
  }
  if (g_wakeFile < 0) {
    fmt::print(fmt::emphasis::bold | fg(fmt::color::red),
               "unable to open wakeFile\n");
  }
  auto saved_flags = fcntl(g_tmpFile, F_GETFL);
  fcntl(g_tmpFile, F_SETFL, saved_flags & ~O_NONBLOCK);

  saved_flags = fcntl(g_wakeFile, F_GETFL);
  fcntl(g_wakeFile, F_SETFL, saved_flags & ~O_NONBLOCK);
  return true;
}
void PipeAdapter::stopThread() {
  g_stop = true;
  if (!_tmpFilename) {
    return;
  }
  write(g_tmpFile, "stop", 4);
  close(g_tmpFile);
  close(g_wakeFile);
  unlink(_tmpFilename);
  free(_tmpFilename);
  _tmpFilename = nullptr;
  if (_worker.joinable()) {
    _worker.join();
  }
}
int PipeAdapter::addMsg(const std::string& msg) {
  std::lock_guard<std::mutex> lock(g_pipeMutex);
  ++g_id;
  g_send.push_back({g_id, msg});
  write(g_wakeFile, "wake", 4);
  return g_id;
}
void PipeAdapter::addResp(int id, const std::string& msg) {
  std::lock_guard<std::mutex> lock(g_pipeMutex);
  g_receive.push_back({id, msg});
  write(g_wakeFile, "wake", 4);
  return;
}
std::list<std::string> PipeAdapter::getAllResp(int id) {
  std::list<std::string> resp;
  fmt::print("lock [{}]\n", __LINE__);
  std::lock_guard<std::mutex> lock(g_pipeMutex);
  fmt::print("release [{}]\n", __LINE__);
  auto itor = g_receive.begin();
  while (itor != g_receive.end()) {
    if (itor->first == id) {
      resp.push_back(itor->second);
      itor = g_receive.erase(itor);
    } else {
      ++itor;
    }
  }
  fmt::print("exit [{}]\n", __LINE__);
  return resp;
}
void waitCommand() {
  struct pollfd fds[1];
  fds[0].fd = g_tmpFile;
  fds[0].events = POLLIN;
  while (!g_stop) {
    int ret = poll(fds, 1, -1);
    if (ret > 0) {
      if (fds[0].revents & POLLIN) {
        std::lock_guard<std::mutex> lock(g_pipeMutex);
        char buf[5] = {0};
        read(g_tmpFile, buf, 4);
        // fmt::print("got [{}]\n", buf);
        if (std::string(buf) == "stop") {
          fmt::print("received stop command\n");
          break;
        }
      }
    }
  }
}
void processCommand() {
  struct pollfd fds[3];
  fds[0].fd = g_to;
  fds[0].events = POLLOUT;
  fds[1].fd = g_from;
  fds[1].events = POLLIN;
  fds[2].fd = g_tmpFile;
  fds[2].events = POLLIN;
  int ret = poll(fds, 3, -1);
  if (ret > 0) {
    if (fds[0].revents & POLLOUT) {
      std::lock_guard<std::mutex> lock(g_pipeMutex);
      // fmt::print("write [{}]\n", __LINE__);
      auto itor = g_send.begin();
      int id = 0;
      if (itor != g_send.end()) {
        id = itor->first;
      }
      while (itor != g_send.end()) {
        if (itor->first == id) {
          std::string msg = fmt::format("Content-Length: {}\r\n\r\n{}",
                                        itor->second.length(), itor->second);
          fmt::print("sending [{}]\n", msg);
          write(g_to, msg.c_str(), msg.length());
          itor = g_send.erase(itor);
        } else {
          ++itor;
        }
      }
    }
    if (fds[1].revents & POLLIN) {
      std::lock_guard<std::mutex> lock(g_pipeMutex);
      std::string msg;
      char buffer[2048] = {0};
      ssize_t bytes_read;
      // fmt::print("read [{}]\n", __LINE__);
      do {
        memset(buffer, 0, sizeof(buffer));
        bytes_read = read(g_from, buffer, sizeof(buffer) - 1);
        if (bytes_read == -1) {
          fmt::print(fmt::emphasis::bold | fg(fmt::color::red),
                     "Error reading from clangd\n");
        } else {
          msg.append(buffer);
        }
      } while (bytes_read == sizeof(buffer) - 1);
      g_receive.push_back({g_id, msg});
    }
    if (fds[2].revents & POLLIN) {
      std::lock_guard<std::mutex> lock(g_pipeMutex);
      char buf[5] = {0};
      read(g_tmpFile, buf, 4);
      // fmt::print("got [{}]\n", buf);
      if (std::string(buf) == "stop") {
        fmt::print("received stop command\n");
      }
    }
  }
}
void PipeAdapter::monitor() {
  while (!g_stop) {
    waitCommand();
    processCommand();
  }
}
PipeAdapter* PipeAdapter::getInstance(const std::string& cmd) {
  if (_instance == nullptr) {
    _instance = new PipeAdapter(cmd);
  }
  return _instance;
}

bool PipeAdapter::initialize() {
  _tmpFilename = nullptr;
  // Create pipes
  if (pipe(_toChild) == -1 || pipe(_fromChild) == -1) {
    fmt::print(fmt::emphasis::bold | fg(fmt::color::red),
               "Pipe creation failed\n");
    return false;
  }
  pid_t pid = fork();
  if (pid == -1) {
    fmt::print(fmt::emphasis::bold | fg(fmt::color::red), "Fork failed\n");
    return false;
  } else if (pid == 0) {   // Child process (clangd)
    close(_toChild[1]);    // Close write end of the pipe to clangd
    close(_fromChild[0]);  // Close read end of the pipe from clangd

    // Redirect pipes to stdin and stdout for clangd
    dup2(_toChild[0], STDIN_FILENO);
    dup2(_fromChild[1], STDOUT_FILENO);

    // Close unused pipe ends
    close(_toChild[0]);
    close(_fromChild[1]);

    // Execute clangd
    execlp(_cmd.c_str(), _cmd.c_str(), NULL);
    fmt::print(fmt::emphasis::bold | fg(fmt::color::red),
               "Failed to execute clangd\n");
    return false;
  } else {                 // Parent process (controller)
    close(_toChild[0]);    // Close read end of the pipe to clangd
    close(_fromChild[1]);  // Close write end of the pipe from clangd

    fmt::print(fmt::emphasis::bold | fg(fmt::color::green), "closed pipes\n");
    fmt::print("make tmpFile [{}]\n", __LINE__);
    makeTempFile();
    fmt::print("Store pipe fd [{}]\n", __LINE__);
    g_to = _toChild[1];
    g_from = _fromChild[0];

    //_worker = std::thread(&PipeAdapter::monitor, this);

    return true;
  }
}

PipeAdapter::PipeAdapter() : _cmd("clangd") {
  fmt::print(fmt::emphasis::bold | fg(fmt::color::green), "Starting client\n");
  initialize();
}

PipeAdapter::PipeAdapter(const std::string& cmd) : _cmd(cmd) {
  initialize();
}

PipeAdapter::~PipeAdapter() {
  stopThread();
  // Close pipe ends
  // close(_toChild[1]);
  // close(_fromChild[0]);
  close(g_to);
  close(g_from);
}

int PipeAdapter::send(const std::string& body) {
  std::string msg =
      fmt::format("Content-Length: {}\r\n\r\n{}", body.length(), body);
  int pipe = _toChild[1];

  write(pipe, msg.c_str(), msg.length());
  // fmt::print("queued message [{}]\n", msg);
  // return addMsg(msg);
  return 0;
}
std::string PipeAdapter::receive() {
  std::string ret;
  int pipe = _fromChild[0];
  char buffer[2048] = {0};
  ssize_t bytes_read;
  do {
    memset(buffer, 0, sizeof(buffer));
    bytes_read = read(pipe, buffer, sizeof(buffer) - 1);
    if (bytes_read == -1) {
      fmt::print(fmt::emphasis::bold | fg(fmt::color::red),
                 "Error reading from clangd\n");
      return "";
    } else {
      ret.append(buffer);
    }
  } while (bytes_read == sizeof(buffer) - 1);

  // fmt::print("received {} \n" ,ret);
  return ParseLSResponse::parse(ret);
  /*fmt::print("queued receive\n");
  auto ret = getAllResp(g_id);
  if (ret.empty()) {
    return "";
  }
  return ret.front();*/
}
