cmake_minimum_required(VERSION 2.8.4)
project(coordinate)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c")

set(SOURCE_FILES geometry.c coordinate.c coordinate_set.c distance_map.c helmert.c string_manip.c task.c formatter/formatter_js.c formatter/formatter_kml.c formatter/formatter_kml_earth.c formatter/formatter_kml_split.c)
add_executable(geometry ${SOURCE_FILES})

add_subdirectory (include/json)
#add_subdirectory (include/php)
include_directories ("include/php")
include_directories ("include/php/zend")