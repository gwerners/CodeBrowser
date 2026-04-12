#pragma once
#include <map>
#include <string>

class Task {
 public:
  int run(const std::string& configFile = "config.json",
          const std::map<std::string, std::string>& overrides = {});
};
