cmake_minimum_required(VERSION 3.10)

project(AutomateDynamatic VERSION 1.0 LANGUAGES C)
set(CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/cmake" ${CMAKE_MODULE_PATH})

set(WARNINGS_LIST "-Wall" "-Wextra" "-Wfloat-equal" "-Wshadow" "-Wpointer-arith"
 "-Wbad-function-cast" "-Wcast-align" "-Wwrite-strings" "-Wconversion" "-Wunreachable-code" 
 "-Wformat=2" "-Wformat-truncation" "-Wstringop-truncation" "-Wformat-overflow" "-Wstringop-overflow"
 "-Wunused-result")

if(CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME)
    set_property(GLOBAL PROPERTY USE_FOLDERS ON)
    set(CMAKE_C_STANDARD 11)
endif()

list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")

add_subdirectory(src)

add_subdirectory(apps)
