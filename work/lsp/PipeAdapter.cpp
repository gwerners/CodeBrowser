#include "lsp/PipeAdapter.h"
#include <poll.h>
#include <string.h>
#include "fmt/color.h"
#include "fmt/format.h"

PipeAdapter* PipeAdapter::_instance = nullptr;

PipeAdapter* PipeAdapter::getInstance(const std::string& cmd) {
  if (_instance == nullptr)
    _instance = new PipeAdapter(cmd);
  return _instance;
}

PipeAdapter::PipeAdapter(const std::string& cmd) : _cmd(cmd) {
  if (!initialize()) {
    fmt::print(fg(fmt::color::red), "Failed to start LSP server: {}\n", cmd);
  }
}

PipeAdapter::~PipeAdapter() {
  close(_toChild[1]);
  close(_fromChild[0]);
}

bool PipeAdapter::initialize() {
  if (pipe(_toChild) == -1 || pipe(_fromChild) == -1) {
    fmt::print(fg(fmt::color::red), "Pipe creation failed\n");
    return false;
  }

  pid_t pid = fork();
  if (pid == -1) {
    fmt::print(fg(fmt::color::red), "Fork failed\n");
    return false;
  }

  if (pid == 0) {
    // Child process - becomes LSP server
    close(_toChild[1]);
    close(_fromChild[0]);
    dup2(_toChild[0], STDIN_FILENO);
    dup2(_fromChild[1], STDOUT_FILENO);
    close(_toChild[0]);
    close(_fromChild[1]);
    // Use sh -c so that _cmd can include arguments (e.g. "cxxlsp /path/index.cxxi")
    execlp("/bin/sh", "/bin/sh", "-c", _cmd.c_str(), nullptr);
    fmt::print(fg(fmt::color::red), "Failed to execute {}\n", _cmd);
    _exit(1);
  }

  // Parent process
  close(_toChild[0]);
  close(_fromChild[1]);
  fmt::print(fg(fmt::color::green), "LSP server started: {}\n", _cmd);
  return true;
}

int PipeAdapter::send(const std::string& body) {
  std::string msg =
      fmt::format("Content-Length: {}\r\n\r\n{}", body.length(), body);
  return write(_toChild[1], msg.c_str(), msg.length());
}

std::string PipeAdapter::sendReceive(const std::string& body) {
  // Try to acquire lock, give up after 2 seconds to avoid blocking server
  std::unique_lock<std::timed_mutex> lock(_mutex, std::defer_lock);
  if (!lock.try_lock_for(std::chrono::seconds(2)))
    return "";
  send(body);
  return receive();
}

void PipeAdapter::sendNotification(const std::string& body) {
  std::unique_lock<std::timed_mutex> lock(_mutex, std::defer_lock);
  if (!lock.try_lock_for(std::chrono::seconds(2)))
    return;
  send(body);
  drain();
}

// Find first complete JSON object with "id" field in a string
static std::string findReplyJson(const std::string& data) {
  std::string::size_type pos = 0;
  while (pos < data.size()) {
    auto start = data.find('{', pos);
    if (start == std::string::npos)
      break;

    int depth = 0;
    bool inStr = false;
    for (auto i = start; i < data.size(); i++) {
      char c = data[i];
      if (inStr) {
        if (c == '\\') { i++; continue; }
        if (c == '"') inStr = false;
      } else {
        if (c == '"') inStr = true;
        else if (c == '{') depth++;
        else if (c == '}') {
          depth--;
          if (depth == 0) {
            std::string js = data.substr(start, i - start + 1);
            if (js.find("\"id\"") != std::string::npos)
              return js;
            pos = i + 1;
            goto next;
          }
        }
      }
    }
    break;  // incomplete JSON
    next:;
  }
  return "";
}

std::string PipeAdapter::receive() {
  std::string accumulated;
  char buffer[4096] = {0};
  struct pollfd pfd = {_fromChild[0], POLLIN, 0};

  // Max 3 seconds total wait for a reply
  for (int i = 0; i < 6; i++) {
    int ready = poll(&pfd, 1, 500);
    if (ready <= 0)
      continue;

    memset(buffer, 0, sizeof(buffer));
    ssize_t n = read(_fromChild[0], buffer, sizeof(buffer) - 1);
    if (n <= 0)
      break;
    accumulated.append(buffer, n);

    std::string reply = findReplyJson(accumulated);
    if (!reply.empty())
      return reply;
  }

  return parseResponse(accumulated);
}

void PipeAdapter::drain() {
  // Read and discard any pending data (notifications, diagnostics)
  char buffer[4096];
  struct pollfd pfd = {_fromChild[0], POLLIN, 0};
  while (poll(&pfd, 1, 100) > 0) {
    if (read(_fromChild[0], buffer, sizeof(buffer)) <= 0)
      break;
  }
}

std::string PipeAdapter::parseResponse(const std::string& msg) {
  // Find the start of the JSON body (after Content-Length header)
  std::size_t pos = msg.find('{');
  if (pos != std::string::npos)
    return msg.substr(pos);
  return msg;
}
