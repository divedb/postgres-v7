cmake_minimum_required(VERSION 3.5 FATAL_ERROR)
project(rdbms C)

set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)
set(CMAKE_C_FLAGS "${CMKAE_C_FLAGS} -g -Wall")
set(RDBMS_INCLUDE_DIR ${PROJECT_SOURCE_DIR}/include)
set(CUNIT_PATH "/usr/local/Cellar/cunit/2.1-3")

include_directories(${CUNIT_PATH}/include)
include_directories(${RDBMS_INCLUDE_DIR})
link_directories(${CUNIT_PATH}/lib)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")
include(ClangTidy)

add_subdirectory(src)
add_subdirectory(test)