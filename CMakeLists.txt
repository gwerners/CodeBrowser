cmake_minimum_required(VERSION 3.16)
project(CodeBrowser VERSION 1.0.0)

set(CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/cmake")

include(FetchContent)

# Skip git checks if deps already fetched (much faster rebuilds)
# First run fetches everything, subsequent runs skip network access
if(EXISTS "${CMAKE_BINARY_DIR}/_deps/fmt-src")
  set(FETCHCONTENT_UPDATES_DISCONNECTED ON)
endif()

# --- fmt ---
FetchContent_Declare(fmt
  GIT_REPOSITORY git@github.com:gwerners/fmt.git
  GIT_TAG        main
  GIT_SHALLOW    TRUE
)
FetchContent_MakeAvailable(fmt)

# --- Crow (header-only HTTP framework) ---
FetchContent_Declare(crow
  GIT_REPOSITORY git@github.com:gwerners/Crow.git
  GIT_TAG        master
  GIT_SHALLOW    TRUE
  PATCH_COMMAND   ${CMAKE_COMMAND}
    -DCROW_DIR=<SOURCE_DIR>
    -P ${CMAKE_SOURCE_DIR}/cmake/patch_crow.cmake
)
FetchContent_Populate(crow)

# --- sol2 (header-only Lua binding) ---
FetchContent_Declare(sol2
  GIT_REPOSITORY git@github.com:gwerners/sol2.git
  GIT_TAG        develop
  GIT_SHALLOW    TRUE
)
FetchContent_Populate(sol2)

# --- Asio (header-only async I/O, required by Crow) ---
FetchContent_Declare(asio
  GIT_REPOSITORY git@github.com:gwerners/asio.git
  GIT_TAG        master
  GIT_SHALLOW    TRUE
)
FetchContent_Populate(asio)

# --- nlohmann/json (header-only, single file) ---
FetchContent_Declare(nlohmann_json
  GIT_REPOSITORY git@github.com:gwerners/json.git
  GIT_TAG        develop
  GIT_SHALLOW    TRUE
)
FetchContent_Populate(nlohmann_json)

# --- tiny-regex-c (only needed with Lucy) ---
FetchContent_Declare(tiny_regex_c
  GIT_REPOSITORY git@github.com:gwerners/tiny-regex-c.git
  GIT_TAG        master
  GIT_SHALLOW    TRUE
)
FetchContent_Populate(tiny_regex_c)

# --- Lucy full-text search (optional) ---
option(USE_LUCY "Build with Apache Lucy full-text search indexing" ON)

if(USE_LUCY)
  FetchContent_Declare(lucy_clownfish
    GIT_REPOSITORY git@github.com:gwerners/lucy-clownfish.git
    GIT_TAG        master
    GIT_SHALLOW    TRUE
  )
  FetchContent_Declare(lucy
    GIT_REPOSITORY git@github.com:gwerners/lucy.git
    GIT_TAG        master
    GIT_SHALLOW    TRUE
  )
  FetchContent_Populate(lucy_clownfish)
  FetchContent_Populate(lucy)

  set(LUCY_CLOWNFISH_SRC "${lucy_clownfish_SOURCE_DIR}" CACHE PATH "")
  set(LUCY_SRC "${lucy_SOURCE_DIR}" CACHE PATH "")
endif()

# Export fetched paths for work/CMakeLists.txt
set(CROW_INCLUDE_DIR "${crow_SOURCE_DIR}/include" CACHE PATH "")
set(SOL2_INCLUDE_DIR "${sol2_SOURCE_DIR}/include" CACHE PATH "")
set(ASIO_INCLUDE_DIR "${asio_SOURCE_DIR}/asio/include" CACHE PATH "")
set(NLOHMANN_INCLUDE_DIR "${nlohmann_json_SOURCE_DIR}/include" CACHE PATH "")
set(TINY_REGEX_DIR "${tiny_regex_c_SOURCE_DIR}" CACHE PATH "")

add_subdirectory(work)
