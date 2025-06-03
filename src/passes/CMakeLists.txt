cmake_minimum_required(VERSION 3.10)
find_package(LLVM 14 REQUIRED CONFIG)
add_definitions(${LLVM_DEFINITIONS})
include_directories(${LLVM_INCLUDE_DIRS})
link_directories(${LLVM_LIBRARY_DIRS})
set(CMAKE_CXX_STANDARD 17)

message(STATUS "Found LLVM version: ${LLVM_VERSION_MAJOR}.${LLVM_VERSION_MINOR}")

add_library(graph SHARED graph.cpp)
add_library(timer_x86 STATIC timer.cpp)

add_library(dfg MODULE dfg_pass.cpp)
add_library(instrument SHARED instrument_pass.cpp)
add_library(prepare SHARED prepare_pass.cpp)
add_library(inliner SHARED inliner.cpp)

target_link_libraries(dfg graph)
