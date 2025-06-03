cmake_minimum_required(VERSION 3.10)
set(CMAKE_CXX_STANDARD 17)
project(PatternMining LANGUAGES C CXX)
set(EXECUTABLE_OUTPUT_PATH "/root/nio/patt_mining/bin/executables")

find_package(LLVM 14 REQUIRED CONFIG)
add_compile_options(-O3)

message(STATUS "Found LLVM version: ${LLVM_VERSION_MAJOR}.${LLVM_VERSION_MINOR}")

add_library(graph graph.cpp)

add_executable(merge merge.cpp)
target_link_libraries(merge graph)
