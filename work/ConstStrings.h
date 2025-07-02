#pragma once

constexpr const char* CONFIG_JSON_FILENAME = "config.json";

// clang-format off
constexpr const char* CONFIG_JSON = R"json(
{
    "url":"http://localhost:3000",
    "port":3000,
    "root": "/home/gwerners/projects/CodeBrowser/root",
    "folders": "folders.html",
    "editor": "editor.html",
    "index": "index.html",
    "history-folder": "history-folder.html",
    "history-file": "history-file.html",
    "diff": "diff.html",
    "search": "search.html",
    "annotate": "annotate.html",
    "projects": [
        {
            "name":"CodeBrowser",
            "source":"/home/gwerners/projects/CodeBrowser",
            "index":"/home/gwerners/projects/CodeWayfinder/index/CodeBrowser-index"
        },
        {
            "name":"three.js",
            "source":"/home/gwerners/projects/three.bgfx/three.js",
            "index":"/home/gwerners/projects/CodeWayfinder/index/three-js-index"
        },
        {
            "name":"tinygltf",
            "source":"/home/gwerners/projects/three.bgfx/tinygltf",
            "index":"/home/gwerners/projects/CodeWayfinder/index/tinygltf-index"
        }
    ],
    "lsp" : "clangd",
    "git" : "/usr/bin/git",
    "cpp-suffix" : ["cpp","c","h","hpp"],
    "update-index" : false
})json";
//"lsp" : "clangd",
//"lsp" :/"/home/gwerners/development/CodeWayfinder/lsp-client/lsp-client/ccls/Release/ccls",
//"lsp" : "lsp_uctags",
//"lsp" : "/home/gwerners/projects/lsp_uctags/build/work/lsp_uctags",
// clang-format on

// MESSAGES
constexpr const char* STARTING_MSG = "Starting server on port {}\n";
constexpr const char* START_UPDATE_INDEX_MSG = "Started index update\n";
constexpr const char* UPDATE_INDEX_MSG =
    "Update index[{}]: path {} source {}\n";
constexpr const char* FINISHED_UPDATE_INDEX_MSG = "Finished index update\n";
constexpr const char* DEFAULT_BODY =
    "<html><body><h5>The URL does not seem to be correct.</body></html>";
