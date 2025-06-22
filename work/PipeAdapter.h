#pragma once
#include <unistd.h>
#include <functional>
#include <list>
#include <mutex>
#include <string>
#include <thread>

#include "nlohmann/json.hpp"
using json = nlohmann::json;

class ParseLSResponse {
 public:
  static const std::string parse(const std::string& msg) {
    std::size_t index = msg.find("{");
    if (index != std::string::npos) {
      auto ret = msg.substr(index);
      return ret;
    }
    return msg;
  }
};

class PipeAdapter {
 public:
  PipeAdapter();
  PipeAdapter(const std::string& cmd);
  ~PipeAdapter();
  int send(const std::string& cmd);
  std::string receive();
  static PipeAdapter* getInstance(const std::string& cmd);
  bool makeTempFile();
  void stopThread();
  int addMsg(const std::string& msg);
  void addResp(int id, const std::string& msg);
  std::list<std::string> getAllResp(int id);
  void monitor();

 private:
  bool initialize();
  std::string _cmd;
  int _toChild[2];    // Pipe for sending data to clangd
  int _fromChild[2];  // Pipe for receiving data from clangd

  char* _tmpFilename;
  std::thread _worker;
};
