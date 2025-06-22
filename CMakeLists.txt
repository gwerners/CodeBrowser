cmake_minimum_required(VERSION 3.16)
project(all-stuff)


set(CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/cmake")

set(WORK_PATH "${PROJECT_SOURCE_DIR}/work")

################################
# unit tests
################################
#add_subdirectory(workUnitTests)


################################
# benchmark
################################
#add_subdirectory(workBenchmark)

################################
# work
################################
add_subdirectory(work)
