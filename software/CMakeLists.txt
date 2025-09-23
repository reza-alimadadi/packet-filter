
cmake_minimum_required(VERSION 3.10)
project(Packet-Filter)

set(CMAKE_BUILD_TYPE "RelWithDebInfo" CACHE STRING "Build type; options are Debug, Release, RelWithDebInfo")

# CMAKE adds -DNDEBUG to release builds by default, which disables assert() calls
# We want to keep assert() calls in RelWithDebInfo builds, so we remove -DNDEBUG from CMAKE_CXX_FLAGS_RELWITHDEBINFO
string(REPLACE "-DNDEBUG" "" CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_CXX_FLAGS_RELWITHDEBINFO}")

find_package(PkgConfig REQUIRED)
find_package(Threads REQUIRED)

pkg_check_modules(DPDK REQUIRED IMPORTED_TARGET libdpdk)

# QDMA library is not in the the .pc file for pkg-config, so we need to find it manually
find_library(QDMA_LIBRARY
    NAMES rte_net_qdma
    PATHS /usr/local/lib/x86_64-linux-gnu)
if(NOT QDMA_LIBRARY)
    message(FATAL_ERROR "Could not find QDMA library")
endif()

# Use relative paths in printed file names easier navigation files
add_compile_options(-ffile-prefix-map=${CMAKE_SOURCE_DIR}=)

# Add DPDK cflags using pkg-config
add_definitions(${DPDK_CFLAGS} ${DPDK_CFLAGS_OTHER})

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall")

set(CMAKE_CXX_FLAGS_DEBUG "-g3 -O0 -DLOG_LEVEL_DEBUG")
set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "-g -O2")
set(CMAKE_CXX_FLAGS_RELEASE "-O3")

# Add include directories
include_directories(${CMAKE_SOURCE_DIR}/src)

# Add source files and exclude executables
file(GLOB_RECURSE SOURCES "src/*.cc")
file(GLOB_RECURSE EXE_SOURCES "src/main.cc")

list(REMOVE_ITEM SOURCES ${EXE_SOURCES})

# Set the output directories
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

add_library("packet_filter" STATIC ${SOURCES})
foreach(EXE_SOURCE ${EXE_SOURCES})
    get_filename_component(EXE_NAME ${EXE_SOURCE} NAME_WE)
    add_executable(${EXE_NAME} ${EXE_SOURCE})
    target_link_libraries(${EXE_NAME} "packet_filter"
                          PkgConfig::DPDK ${QDMA_LIBRARY}
                          Threads::Threads)
endforeach()

