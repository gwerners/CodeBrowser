cmake_minimum_required(VERSION 3.16)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -O2 -Wall")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -g -O2 -Wall")

set(MY_PROJECT_NAME codebrowser)

# Embedded Lua (minilua single-file, kept locally)
set(MINILUA_SRC ${CMAKE_CURRENT_SOURCE_DIR}/minilua.c)

# Core sources (always compiled)
set(SRC
  main.cpp
  Task.cpp
  Server.cpp
  Config.cpp
  Utils.cpp
  Git.cpp
  lsp/LspClient.cpp
  lsp/PipeAdapter.cpp
  ctags/CtagsProvider.cpp
  indexer/GrepSearcher.cpp)

# Lucy full-text search (optional)
if(USE_LUCY)
  list(APPEND SRC
    indexer/FullTextIndexer.cpp
    indexer/FullTextSearcher.cpp)

  set(TINY_REGEXP_SRC ${TINY_REGEX_DIR}/re.c)
  include_directories(${TINY_REGEX_DIR})

  # Lucy/Clownfish installed libs (built by run.bash)
  include_directories(${CMAKE_SOURCE_DIR}/env/include)
  link_directories(${CMAKE_SOURCE_DIR}/env/lib)

  add_definitions(-DUSE_LUCY)
  message(STATUS "Building WITH Apache Lucy full-text search")
else()
  message(STATUS "Building WITHOUT Lucy - using grep/ripgrep for search")
endif()

# Include paths - fetched dependencies
include_directories(${CMAKE_CURRENT_SOURCE_DIR})
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/lua)  # lua.h wrappers -> minilua.h
include_directories(${CROW_INCLUDE_DIR})
include_directories(${SOL2_INCLUDE_DIR})
include_directories(${ASIO_INCLUDE_DIR})
include_directories(${NLOHMANN_INCLUDE_DIR})

if(USE_LUCY)
  add_executable(${MY_PROJECT_NAME} ${SRC} ${MINILUA_SRC} ${TINY_REGEXP_SRC})
  target_link_libraries(${MY_PROJECT_NAME} fmt::fmt pthread clownfish lucy dl)
else()
  add_executable(${MY_PROJECT_NAME} ${SRC} ${MINILUA_SRC})
  target_link_libraries(${MY_PROJECT_NAME} fmt::fmt pthread dl)
endif()

# Optional clang-format integration
if(EXISTS "${CMAKE_SOURCE_DIR}/cmake/gwf_module.cmake")
  include(gwf_module)
  fix_format(${CMAKE_CURRENT_SOURCE_DIR} ${SRC})
endif()
