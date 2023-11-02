cmake_minimum_required(VERSION 3.5 FATAL_ERROR)
project(rdbms C)

# set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)
set(CMAKE_C_FLAGS "${CMKAE_C_FLAGS} -g -Wall")
set(RDBMS_INCLUDE_DIR ${PROJECT_SOURCE_DIR}/include)

find_package(CUNIT QUIET)

if(NOT cunit_FOUND)
    find_package(PkgConfig)
    pkg_check_modules(CUNIT REQUIRED IMPORTED_TARGET cunit)
endif()

include_directories(${RDBMS_INCLUDE_DIR})
list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")
include(ClangTidy)

add_subdirectory(src)
add_subdirectory(test)