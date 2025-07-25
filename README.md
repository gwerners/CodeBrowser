![ ██████╗ ██████╗ ██████╗ ███████╗██████╗ ██████╗  ██████╗ ██╗    ██╗███████╗███████╗██████╗ 
██╔════╝██╔═══██╗██╔══██╗██╔════╝██╔══██╗██╔══██╗██╔═══██╗██║    ██║██╔════╝██╔════╝██╔══██╗
██║     ██║   ██║██║  ██║█████╗  ██████╔╝██████╔╝██║   ██║██║ █╗ ██║███████╗█████╗  ██████╔╝
██║     ██║   ██║██║  ██║██╔══╝  ██╔══██╗██╔══██╗██║   ██║██║███╗██║╚════██║██╔══╝  ██╔══██╗
╚██████╗╚██████╔╝██████╔╝███████╗██████╔╝██║  ██║╚██████╔╝╚███╔███╔╝███████║███████╗██║  ██║
 ╚═════╝ ╚═════╝ ╚═════╝ ╚══════╝╚═════╝ ╚═╝  ╚═╝ ╚═════╝  ╚══╝╚══╝ ╚══════╝╚══════╝╚═╝  ╚═╝](https://github.com/gwerners/CodeBrowser/blob/main/banner.png?raw=true)



# CodeBrowser

A simple code navigation and search tool inspired by [OpenGrok](https://github.com/oracle/opengrok).   
This project is still under development — many features are missing or disabled. Only basic functionality is available, so use it at your own risk.

## Getting Started

To run the server:

1. Execute `./run.bash`. This script will build the project, install dependencies, and start the server.
2. Open your browser at [http://localhost:3000/](http://localhost:3000/) to access the index page.

## Interface

![Search View](https://github.com/gwerners/CodeBrowser/blob/main/images/Search.png?raw=true)

The **Search View** is the initial screen you will see after opening [http://localhost:3000/](http://localhost:3000/). It allows you to search for strings within the selected project. After clicking the **Search** button, a list of results will appear, with each entry linking directly to the corresponding file and line.

---

![Editor View](https://github.com/gwerners/CodeBrowser/blob/main/images/Editor.png?raw=true)

Clicking a link from the search results will open the file in an embedded code editor powered by [Monaco](https://microsoft.github.io/monaco-editor/), the same editor used by VS Code. It includes basic navigation features — for example, right-click to access the context menu. (Note: not all features are bug-free at this stage.)

---

![Blame View](https://github.com/gwerners/CodeBrowser/blob/main/images/Annotated.png?raw=true)

From the **Editor View**, you can select **Annotate** to open the **Blame View**. This view displays each line along with the corresponding commit hash, author, and timestamp of the last change.

---

![File Revision History View](https://github.com/gwerners/CodeBrowser/blob/main/images/FileRevision.png?raw=true)

By selecting **History** from the **Editor View**, you access the **File Revision History View**. This shows a list of past revisions of the current file. You can click a revision hash to view its contents or select two revisions using radio buttons to compare them in the **Diff View**.

---

![Diff View](https://github.com/gwerners/CodeBrowser/blob/main/images/Diff.png?raw=true)

The **Diff View** displays a side-by-side comparison of two revisions using embedded Monaco editors. It highlights the differences between the selected versions of a file.

---

![Folder View](https://github.com/gwerners/CodeBrowser/blob/main/images/FolderView.png?raw=true)

You can access the **Folder View** in two ways: by double-clicking the project name in the **Search View**, or by clicking a directory name in the **Editor View**. This view displays the contents of a directory, allowing you to navigate through the project structure.  
Click the current folder name to go up one level, or click a folder link to open that subdirectory.

---

![Folder Revision History View](https://github.com/gwerners/CodeBrowser/blob/main/images/FolderRevision.png?raw=true)

Clicking **History** while in the **Folder View** shows the revision history of all files in that directory. Clicking **Show modified files** reveals the changed files for each revision, allowing you to navigate directly to each file version.


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
