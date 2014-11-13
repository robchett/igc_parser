cmake_minimum_required(VERSION 2.8.4)
project(json)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c")

set(SOURCE_FILES dump.c hashtable.c hashtable_seed.c strconv.c utf.c value.c)