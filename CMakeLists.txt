#
# MIT License
#
# Copyright (c) 2022 Nora Khayata
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in all
# copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
# SOFTWARE.

cmake_minimum_required(VERSION 3.14)
cmake_policy(SET CMP0075 NEW)
project(FUSE VERSION 0.1.0)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DFLATBUFFERS_USE_STD_SPAN") # add std::span support to flatbuffers

set(FUSE_ROOT_DIR "${CMAKE_CURRENT_SOURCE_DIR}")
set(FUSE_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/src")
set(FUSE_EXTERN_DIR "${FUSE_ROOT_DIR}/extern")
set(FUSE_TEST_DIR "${CMAKE_CURRENT_SOURCE_DIR}/tests")
set(FUSE_BENCHMARKS_DIR "${CMAKE_CURRENT_SOURCE_DIR}/benchmarks")
set(FUSE_FBS_INCLUDE_PREFIX "${FUSE_SOURCE_DIR}/core/fuse_ir_fb_headers")

set(FUSE_FLATBUFFERS_SRC_DIR "${FUSE_EXTERN_DIR}/flatbuffers")
set(FUSE_FLATBUFFERS_INCLUDE_DIR "${FUSE_FLATBUFFERS_SRC_DIR}/include")
set(FUSE_HyCC_SRC_DIR "${FUSE_EXTERN_DIR}/HyCC")
set(FUSE_DISTGRAPH_SRC_DIR "${FUSE_EXTERN_DIR}/DistGraph")
set(FUSE_GLASGOW_SRC_DIR "${FUSE_EXTERN_DIR}/glasgow-subgraph-solver")
set(FUSE_MOTION_SRC_DIR "${FUSE_EXTERN_DIR}/MOTION")
set(FUSE_BENCHMARK_SRC_DIR "${FUSE_EXTERN_DIR}/benchmark")
set(FUSE_GZIP_DIR "${FUSE_EXTERN_DIR}/gzip-hpp")
set(FUSE_GZIP_INCLUDE_DIR "${FUSE_EXTERN_DIR}/gzip-hpp/include")
set(FUSE_TUTORIAL_DIR "${FUSE_SOURCE_DIR}/examples")


option(FUSE_BUILD_TESTS "Build Tests" OFF)
option(FUSE_BUILD_BENCHMARKS "Build Benchmarks" ON)
option(FUSE_USE_ADDRESS_SANITIZER "Enable compile and link options for address sanitizer" OFF)
option(FUSE_BUILD_BRISTOL_FRONTEND "Build Bristol Frontend" ON)
option(FUSE_BUILD_HyCC_FRONTEND "Build HyCC Frontend" ON)
option(FUSE_BUILD_MOTION_FRONTEND "Build MOTION Frontend" ON)
option(FUSE_BUILD_MOTION_BACKEND "Build MOTION Backend" ON)
option(FUSE_BUILD_TUTORIALS "Build FUSE Tutorials for Devs" ON)
option(FUSE_BUILD_SUBGRAPH_OPTIMIZATION "Build Subgraph Optimization" ON)


set(CMAKE_BUILD_TYPE Debug)
set(CMAKE_CXX_FLAGS_DEBUG "-g")

# if (FUSE_BUILD_BENCHMARKS)
# set(CMAKE_BUILD_TYPE Release)

# endif()

# Write built executables and libraries to bin/ and lib/, respectively.
if(NOT CMAKE_RUNTIME_OUTPUT_DIRECTORY)
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/bin")
endif()

if(NOT CMAKE_LIBRARY_OUTPUT_DIRECTORY)
    set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/lib")
endif()

if(NOT CMAKE_ARCHIVE_OUTPUT_DIRECTORY)
    set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/lib")
endif()

# #################### Add Libaries used inside FUSE #####################

# Add gzip library for compressing files
message(STATUS "Add gzip library")

if(NOT EXISTS "${FUSE_GZIP_DIR}/CMakeLists.txt")
    find_package(Git REQUIRED)
    message("initialize Git submodule: extern/gzip-hpp")
    execute_process(COMMAND git submodule update --init extern/gzip-hpp
        WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}")
endif()

add_subdirectory(${FUSE_GZIP_DIR} EXCLUDE_FROM_ALL)

# Add MOTION if the Backend or Frontend is to be built
if(FUSE_BUILD_MOTION_BACKEND OR FUSE_BUILD_MOTION_FRONTEND)
    find_package(motion QUIET)

    if(motion_FOUND)
        message(STATUS "Found MOTION")

    elseif(NOT motion_FOUND AND NOT TARGET motion::motion)
        message(STATUS "Building MOTION")

        # Build MOTION
        if(NOT EXISTS "${PROJECT_SOURCE_DIR}/extern/MOTION/CMakeLists.txt")
            find_package(Git REQUIRED)
            message("initialize Git submodule: extern/MOTION")
            execute_process(COMMAND git submodule update --init extern/MOTION
                WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}")
        endif()

        # Update Flatbuffers inside MOTION to newest version, so that there are no conflicts
        message("update flatbuffers inside MOTION")
        execute_process(COMMAND git fetch
                        COMMAND git pull origin master
                        WORKING_DIRECTORY "${FUSE_MOTION_SRC_DIR}/extern/flatbuffers")

        add_subdirectory(${FUSE_MOTION_SRC_DIR})
    endif()
endif()

# Add flatbuffers
find_package(Flatbuffers QUIET)

if(Flatbuffers_FOUND)
    message(STATUS "Found flatbuffers")

    if(WIN32)
        set(FLATBUFFERS_FLATC_EXECUTABLE "flatc.exe")
    else()
        set(FLATBUFFERS_FLATC_EXECUTABLE "flatc")
    endif()

elseif(NOT Flatbuffers_FOUND AND NOT TARGET flatbuffers::flatbuffers)
    message("flatbuffers was not found: compile flatc")

    if(NOT EXISTS "${PROJECT_SOURCE_DIR}/extern/flatbuffers/CMakeLists.txt")
        find_package(Git REQUIRED)
        message("initialize Git submodule: extern/flatbuffers")
        execute_process(COMMAND git submodule update --init extern/flatbuffers
            WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}")
    endif()

    set(FLATBUFFERS_BUILD_FLATC ON CACHE BOOL "Enable the build of the flatbuffers compiler" FORCE)
    set(FLATBUFFERS_BUILD_FLATLIB ON CACHE BOOL "Enable building flatlib" FORCE)
    set(FLATBUFFERS_BUILD_FLATHASH OFF CACHE BOOL "Disable building flathash" FORCE)
    set(FLATBUFFERS_BUILD_TESTS OFF CACHE BOOL "Disable building flatbuffers tests" FORCE)

    add_subdirectory(${FUSE_FLATBUFFERS_SRC_DIR}
        ${CMAKE_CURRENT_BINARY_DIR}/flatbuffers-build
        EXCLUDE_FROM_ALL)

    if(WIN32)
        set(FLATBUFFERS_FLATC_EXECUTABLE "${PROJECT_BINARY_DIR}/bin/flatc.exe")
    else()
        set(FLATBUFFERS_FLATC_EXECUTABLE "${PROJECT_BINARY_DIR}/bin/flatc")
    endif()
endif()

# #################### Add subdirectories and link to FUSE target #####################

# Add generated fbs headers
add_subdirectory(fbs)

# Add project sources, this defines the FUSE target
add_subdirectory(${FUSE_SOURCE_DIR})

# Wait until the fbs headers and libraries are generated before generating the FUSE libary
add_dependencies(FUSE FUSE-fbs-headers-generation)

# Link against flatbuffers to already declared target.
target_link_libraries(FUSE PUBLIC flatbuffers)
target_include_directories(FUSE PUBLIC ${FUSE_FLATBUFFERS_INCLUDE_DIR} ${FUSE_FBS_INCLUDE_PREFIX})

add_subdirectory(src/frontend)
add_subdirectory(src/backend)

# #################### Build tests, benchmarks, tutorials if enabled #####################
if(FUSE_BUILD_TUTORIALS)
    add_subdirectory(${FUSE_TUTORIAL_DIR})
endif(FUSE_BUILD_TUTORIALS)

# CMakeLists content taken from https://google.github.io/googletest/quickstart-cmake.html
if(FUSE_BUILD_TESTS)
    include(FetchContent)
    FetchContent_Declare(
        googletest
        URL https://github.com/google/googletest/archive/609281088cfefc76f9d0ce82e1ff6c30cc3591e5.zip
    )

    # For Windows: Prevent overriding the parent project's compiler/linker settings
    set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(googletest)

    enable_testing()
    add_subdirectory(${FUSE_TEST_DIR})
endif(FUSE_BUILD_TESTS)

# Add google benchmarking library
if(FUSE_BUILD_BENCHMARKS)
    if(NOT EXISTS "${FUSE_BENCHMARK_SRC_DIR}/CMakeLists.txt")
        find_package(Git REQUIRED)
        message("initialize Git submodule: extern/benchmark")
        execute_process(COMMAND git submodule update --init extern/benchmark
            WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}")
    endif()

    set(BENCHMARK_ENABLE_TESTING OFF CACHE BOOL "Don't build tests for the benchmark library")
    set(BENCHMARK_ENABLE_INSTALL ON CACHE BOOL "Don't install the benchmark library")

    add_subdirectory(${FUSE_BENCHMARK_SRC_DIR} EXCLUDE_FROM_ALL)
    add_subdirectory(${FUSE_BENCHMARKS_DIR})
endif(FUSE_BUILD_BENCHMARKS)

set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)
