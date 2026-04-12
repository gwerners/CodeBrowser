#include <cstring>
#include <iostream>
#include <map>
#include <string>
#include "Task.h"

static void printUsage(const char* progname) {
  std::cout
      << "Usage: " << progname << " [options]\n"
      << "\n"
      << "CodeBrowser - Source code navigation server\n"
      << "\n"
      << "Options:\n"
      << "  -h, --help                  Show this help message\n"
      << "  -c, --config <file>         Config file path (default: config.json)\n"
      << "  -p, --port <port>           Server port\n"
      << "  -u, --url <url>             Server URL (e.g. http://localhost:3000)\n"
      << "  -r, --root <path>           HTML root directory\n"
      << "  -g, --git <path>            Git binary path\n"
      << "  -l, --lsp <path>            LSP server binary (e.g. clangd)\n"
      << "  -s, --symbol-provider <p>   Symbol provider: clangd or ctags\n"
      << "  -e, --search-engine <e>     Search engine: lucy or grep\n"
      << "  -t, --theme <theme>         Theme: light or dark\n"
      << "  --update-index              Force index update on startup\n"
      << "  --no-update-index           Skip index update on startup\n"
      << "\n"
      << "Command line options override config.json values.\n"
      << "Config.json overrides built-in defaults.\n";
}

int main(int argc, char** argv) {
  std::string configFile = "config.json";
  std::map<std::string, std::string> overrides;

  for (int i = 1; i < argc; i++) {
    std::string arg = argv[i];

    if (arg == "-h" || arg == "--help") {
      printUsage(argv[0]);
      return 0;
    }

    // Flags with argument
    auto nextArg = [&]() -> std::string {
      if (i + 1 < argc)
        return argv[++i];
      std::cerr << "Error: " << arg << " requires an argument\n";
      return "";
    };

    if (arg == "-c" || arg == "--config") {
      configFile = nextArg();
    } else if (arg == "-p" || arg == "--port") {
      overrides["port"] = nextArg();
    } else if (arg == "-u" || arg == "--url") {
      overrides["url"] = nextArg();
    } else if (arg == "-r" || arg == "--root") {
      overrides["root"] = nextArg();
    } else if (arg == "-g" || arg == "--git") {
      overrides["git"] = nextArg();
    } else if (arg == "-l" || arg == "--lsp") {
      overrides["lsp"] = nextArg();
    } else if (arg == "-s" || arg == "--symbol-provider") {
      overrides["symbol-provider"] = nextArg();
    } else if (arg == "-e" || arg == "--search-engine") {
      overrides["search-engine"] = nextArg();
    } else if (arg == "-t" || arg == "--theme") {
      overrides["theme"] = nextArg();
    } else if (arg == "--update-index") {
      overrides["update-index"] = "true";
    } else if (arg == "--no-update-index") {
      overrides["update-index"] = "false";
    } else {
      std::cerr << "Unknown option: " << arg << "\n";
      printUsage(argv[0]);
      return 1;
    }
  }

  Task task;
  return task.run(configFile, overrides);
}
