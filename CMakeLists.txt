# Copyright (c) 2019 University of Maryland. All Rights Reserve.

cmake_minimum_required(VERSION 3.0)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

project(chord VERSION 0.1.0 LANGUAGES C CXX)

set(CHORD_SOURCE_DIR   ${CMAKE_CURRENT_SOURCE_DIR})
set(CHORD_BINARY_DIR   ${CMAKE_CURRENT_BINARY_DIR})

include(systems)
include(bazel)          # simplify cmake build interface
include(compile)        # configure the compilation flag

include(external/protobuf)
include(external/openssl)
include(external/gflags)
include(external/glog)

include_directories(${CHORD_SOURCE_DIR})

add_subdirectory(chord)
