#include "Task.h"
#include <sys/wait.h>
#include "Server.h"

#ifdef USING_QT
Task::Task(QObject* parent) : QObject(parent) {}
#else
Task::Task() {}
#endif

static void func(int) {
  wait(NULL);
}

int Task::run() {
  signal(SIGCHLD, func);
  Server server;
  server.run();
#ifdef USING_QT
  emit finished();
#endif
  return 0;
}
