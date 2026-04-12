#pragma once
#include <mutex>
#include <unistd.h>
#include <string>

// Manages a bidirectional pipe to an LSP server process (e.g. clangd).
// Singleton pattern - one LSP server per application instance.
class PipeAdapter {
 public:
  ~PipeAdapter();

  int send(const std::string& body);
  std::string receive();
  void drain();
  std::string sendReceive(const std::string& body);      // Atomic send+receive (for requests)
  void sendNotification(const std::string& body);         // Atomic send+drain (for notifications)

  static PipeAdapter* getInstance(const std::string& cmd);

 private:
  PipeAdapter(const std::string& cmd);
  bool initialize();

  // Parse LSP response: skip Content-Length header, return JSON body
  static std::string parseResponse(const std::string& msg);

  std::string _cmd;
  int _toChild[2];
  int _fromChild[2];
  std::timed_mutex _mutex;  // Serialize access to pipe

  static PipeAdapter* _instance;
};
