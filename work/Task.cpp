#include "Task.h"
#include <signal.h>
#include <sys/wait.h>
#include "Server.h"

static void reapChildren(int) {
  wait(nullptr);
}

int Task::run(const std::string& configFile,
              const std::map<std::string, std::string>& overrides) {
  signal(SIGCHLD, reapChildren);
  Server server(configFile, overrides);
  server.run();
  return 0;
}
