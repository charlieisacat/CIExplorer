cmake_minimum_required(VERSION 3.10)
set(CMAKE_CXX_STANDARD 17)
project(PatternMining LANGUAGES C CXX)
set(EXECUTABLE_OUTPUT_PATH "/root/nio/patt_mining/bin/executables")

find_package(LLVM 14 REQUIRED CONFIG)
add_compile_options(-O3)

add_definitions(${LLVM_DEFINITIONS})
include_directories(${LLVM_INCLUDE_DIRS})
link_directories(${LLVM_LIBRARY_DIRS})

find_package(yaml-cpp REQUIRED)

add_library(graph SHARED ../mining/graph.cpp)
add_library(pm ../mining/PatternManager.cpp)
add_library(utils ../mining/utils.cpp)
add_library(cost_model ../cost_model/cost_model.cpp)
add_library(yaml_parser ../cost_model/yaml_parser.cpp)

add_executable(parse_power_area parse_power_area.cpp)
add_executable(parse_power_area_needle parse_power_area_needle.cpp)

target_link_libraries(parse_power_area graph pm utils cost_model)
target_link_libraries(parse_power_area_needle graph pm utils cost_model)

llvm_map_components_to_libnames(llvm_libs support core irreader)
target_link_libraries(graph ${llvm_libs})
target_link_libraries(pm ${llvm_libs})
target_link_libraries(cost_model ${llvm_libs} pm graph yaml_parser)

target_link_libraries(yaml_parser yaml-cpp)

