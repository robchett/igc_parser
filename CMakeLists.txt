cmake_minimum_required(VERSION 3.3)
project(igc_parser)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

add_custom_target(igc_parser COMMAND make -C ${igc_parser_SOURCE_DIR} CLION_EXE_DIR=${PROJECT_BINARY_DIR})
add_custom_target(igc_parser_clean COMMAND make clean -C ${igc_parser_SOURCE_DIR} CLION_EXE_DIR=${PROJECT_BINARY_DIR})