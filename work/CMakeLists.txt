cmake_minimum_required(VERSION 3.25.1)

set(CMAKE_EXPORT_COMPILE_COMMANDS true)

CMAKE_POLICY(SET CMP0054 NEW) #-- fixes IF() with quoted args
CMAKE_POLICY(SET CMP0057 NEW) #-- enables IF(.. IN_LIST ..)

set(MY_PROJECT_NAME lsp-client)
project(
  ${MY_PROJECT_NAME}
  VERSION 1.0.0
)
include(CheckCXXCompilerFlag)

CHECK_CXX_COMPILER_FLAG("-std=c++20" COMPILER_SUPPORTS_CXX20)
CHECK_CXX_COMPILER_FLAG("-std=c++17" COMPILER_SUPPORTS_CXX17)
CHECK_CXX_COMPILER_FLAG("-std=c++14" COMPILER_SUPPORTS_CXX14)
CHECK_CXX_COMPILER_FLAG("-std=c++11" COMPILER_SUPPORTS_CXX11)
CHECK_CXX_COMPILER_FLAG("-std=c++0x" COMPILER_SUPPORTS_CXX0X)
if(COMPILER_SUPPORTS_CXX20)
    message(STATUS "The compiler ${CMAKE_CXX_COMPILER} supports C++20")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++20")
    set(CMAKE_CXX_STANDARD 20)
    set(MY_COMPILER_FEATURES cxx_std_20)
elseif(COMPILER_SUPPORTS_CXX17)
    message(STATUS "The compiler ${CMAKE_CXX_COMPILER} supports C++17")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17")
    set(CMAKE_CXX_STANDARD 17)
    set(MY_COMPILER_FEATURES cxx_std_17)
elseif(COMPILER_SUPPORTS_CXX14)
    message(STATUS "The compiler ${CMAKE_CXX_COMPILER} supports C++14")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++14")
    set(CMAKE_CXX_STANDARD 14)
    set(MY_COMPILER_FEATURES cxx_std_14)
elseif(COMPILER_SUPPORTS_CXX11)
    message(STATUS "The compiler ${CMAKE_CXX_COMPILER} supports C++11")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
    set(CMAKE_CXX_STANDARD 11)
    set(MY_COMPILER_FEATURES cxx_std_11)
elseif(COMPILER_SUPPORTS_CXX0X)
     message(STATUS "The compiler ${CMAKE_CXX_COMPILER} supports C++0x")
     set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++0x")
else()
     message(STATUS "The compiler ${CMAKE_CXX_COMPILER} has no C++11 support. Please use a different C++ compiler.")
endif()

set( CMAKE_VERBOSE_MAKEFILE on )

#set(CMAKE_CXX_FLAGS $ENV{CXXFLAGS})
#set(CMAKE_CXX_FLAGS_RELEASE $ENV{CXXFLAGS})
#message("CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS}")
#message("CMAKE_CXX_FLAGS_RELEASE ${CMAKE_CXX_FLAGS_RELEASE}")
#message("CXXFLAGS $ENV{CXXFLAGS}")

#set(CMAKE_STATIC_LINKER_FLAGS "--coverage -fprofile-arcs -ftest-coverage -lgcov")
#set(CMAKE_CXX_FLAGS "-g -O0 -Wall --coverage -fprofile-arcs -ftest-coverage")
#set(CMAKE_C_FLAGS "-g -O0 -Wall -W --coverage -fprofile-arcs -ftest-coverage")
#set(CMAKE_EXE_LINKER_FLAGS "--coverage -fprofile-arcs -ftest-coverage")

set(CMAKE_CXX_FLAGS "-g -O2 -Wall")
set(CMAKE_C_FLAGS "-g -O2 -Wall -W")

#include_directories(${CMAKE_SOURCE_DIR}/curl-7.75.0/include)
#include_directories(${CMAKE_SOURCE_DIR}/pistache/include)
#include_directories(${CMAKE_SOURCE_DIR}/json/single_include)
#include_directories(${CMAKE_SOURCE_DIR}/compile-time-regular-expressions/include)
#include_directories(${CMAKE_SOURCE_DIR}/cpp-taskflow)
#include_directories(${CMAKE_SOURCE_DIR}/sqlite-amalgamation-3350000)
#include_directories(/usr/include/python3.9)

#link_directories(${CMAKE_SOURCE_DIR}/curl-7.75.0/build/lib)
#link_directories(${CMAKE_SOURCE_DIR}/pistache/build/src)
#link_directories(/usr/local/lib)
include(FetchContent)

# Formatting library
set(FMT
  ${CMAKE_SOURCE_DIR}/fmt-src/format.cc
  ${CMAKE_SOURCE_DIR}/fmt-src/os.cc)

set(SOL
  ${CMAKE_SOURCE_DIR}/sol/minilua.c
)

set(SRC
  main.cpp
  Server.cpp
  Task.cpp
  Utils.cpp
  Git.cpp
  FullTextIndexer.cpp
  FullTextSearcher.cpp
  PipeAdapter.cpp)

set(HEADERS
  ConstStrings.h
  LSMessage.h
  PipeCommand.h
  Server.h
  Task.h
  Utils.h
  Git.h
  FullTextIndexer.h
  FullTextSearcher.h
  SearchFile.h
  SearchInfo.h
  SearchLine.h
  PipeAdapter.h)

include_directories(${CMAKE_SOURCE_DIR})
include_directories(${CMAKE_SOURCE_DIR}/fmt)
#include_directories(${CMAKE_SOURCE_DIR}/sol)
include_directories(${CMAKE_SOURCE_DIR}/crow)
include_directories(${CMAKE_SOURCE_DIR}/asio)

include_directories(/usr/lib/llvm-18/include)
link_directories(/usr/lib/llvm-18/lib)

include_directories(${CMAKE_SOURCE_DIR}/env/include)
link_directories(${CMAKE_SOURCE_DIR}/env/lib)

add_executable(${MY_PROJECT_NAME} ${SRC} ${FMT} ${SOL})
target_link_libraries(${MY_PROJECT_NAME} pthread clownfish lucy clang dl)
target_compile_features(${MY_PROJECT_NAME} PRIVATE ${MY_COMPILER_FEATURES})

#module with helper functions
include(gwf_module)

fix_format(${CMAKE_SOURCE_DIR}/work ${SRC} )
fix_format(${CMAKE_SOURCE_DIR}/work ${HEADERS} )

