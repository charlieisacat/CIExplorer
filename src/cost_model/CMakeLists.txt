cmake_minimum_required(VERSION 3.10)
set(CMAKE_CXX_STANDARD 17)
project(PatternMining LANGUAGES C CXX)
# set(EXECUTABLE_OUTPUT_PATH "/root/git/patt_mining_llvm/bin/executables")

find_package(LLVM REQUIRED CONFIG)
add_compile_options(-O3)

find_package(yaml-cpp REQUIRED)

add_definitions(${LLVM_DEFINITIONS})
include_directories(${LLVM_INCLUDE_DIRS})
link_directories(${LLVM_LIBRARY_DIRS})

add_library(packing packing.cpp)
add_library(pm ../mining/PatternManager.cpp)
add_library(graph SHARED ../mining/graph.cpp)
add_library(utils ../mining/utils.cpp)
add_library(cost_model cost_model.cpp)
add_library(yaml_parser yaml_parser.cpp)


add_executable(test test.cpp)

target_link_libraries(test packing graph pm utils cost_model)

llvm_map_components_to_libnames(llvm_libs support core irreader)
target_link_libraries(graph ${llvm_libs})
target_link_libraries(packing ${llvm_libs})
target_link_libraries(pm ${llvm_libs})
target_link_libraries(utils ${llvm_libs})
target_link_libraries(cost_model ${llvm_libs} pm graph yaml_parser)

target_link_libraries(yaml_parser yaml-cpp)