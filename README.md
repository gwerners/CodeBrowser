[ ██████╗ ██████╗ ██████╗ ███████╗██████╗ ██████╗  ██████╗ ██╗    ██╗███████╗███████╗██████╗ 
██╔════╝██╔═══██╗██╔══██╗██╔════╝██╔══██╗██╔══██╗██╔═══██╗██║    ██║██╔════╝██╔════╝██╔══██╗
██║     ██║   ██║██║  ██║█████╗  ██████╔╝██████╔╝██║   ██║██║ █╗ ██║███████╗█████╗  ██████╔╝
██║     ██║   ██║██║  ██║██╔══╝  ██╔══██╗██╔══██╗██║   ██║██║███╗██║╚════██║██╔══╝  ██╔══██╗
╚██████╗╚██████╔╝██████╔╝███████╗██████╔╝██║  ██║╚██████╔╝╚███╔███╔╝███████║███████╗██║  ██║
 ╚═════╝ ╚═════╝ ╚═════╝ ╚══════╝╚═════╝ ╚═╝  ╚═╝ ╚═════╝  ╚══╝╚══╝ ╚══════╝╚══════╝╚═╝  ╚═╝](https://raw.githubusercontent.com/gwerners/CodeBrowser/master/banner.jpg)



# CodeBrowser

A simple code navigation and search tool inspired by [OpenGrok](https://github.com/oracle/opengrok).   
This project is still under development — many features are missing or disabled. Only basic functionality is available, so use it at your own risk.

## Getting Started

To run the server:

1. Execute `./run.bash`. This script will build the project, install dependencies, and start the server.
2. Open your browser at [http://localhost:3000/](http://localhost:3000/) to access the index page.

### Configuration via JSON

The project uses a JSON configuration inside the `ConstStrings.h` file to manage paths and settings for indexing:

- `"root"`: Location of HTML templates.
- `"files"`: Directory containing static files (e.g., CSS, JS).
- `"source"`: Path to the source code you want to index.
- `"index"`: Directory where the search index will be created.
- `"update-index"`: Set to `false` if your project is large, to prevent reindexing every time the server starts.

> Make sure these paths are correctly set before running the server for the first time.

## Prerequisites

This project uses the following libraries (all included in the repository for simplicity):

- [Asio](https://think-async.com/) 
- [Crow](https://github.com/ipkn/crow) 
- [fmt](https://github.com/fmtlib/fmt) 
- [nlohmann/json](https://github.com/nlohmann/json) 
- [Sol2](https://github.com/ThePhD/sol2) 
- [Lua](https://www.lua.org/) 

> Note: These are currently bundled with the project. Future updates may switch to using CMake or Git submodules for dependency management.

## Installation

Currently, the server runs as a standalone application without requiring system-wide installation.  
More detailed installation scripts and documentation will be added in future releases.

## Authors

- **Gabriel Wernersbach** - *Initial work* - [gwerners](https://github.com/gwerners) 

## License

This project is licensed under the MIT License – see the [LICENSE.md](LICENSE.md) file for details.*
