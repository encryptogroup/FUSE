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

########################### Define files, link libaries and include directories ###########################

# "barebone" definition
set(FUSE_TEST_CPP
    # TestWrappers.cpp
    # TestInterpreter.cpp
    # TestDeadNodeOptimization.cpp
    # TestConstantFolding.cpp
    # TestDepthAnalysis.cpp
    # TestLargeCircuits.cpp
    # TestInstructionVectorization.cpp
    TestDOTBackend.cpp
    # TestMnistCirc.cpp
    )
set (FUSE_TEST_LIBS
    FUSE
    FUSE_Bristol
    gtest_main
    flatbuffers)
set (FUSE_TEST_INCLUDE_DIRS
    ${FUSE_FBS_INCLUDE_PREFIX}
    ${FUSE_FLATBUFFERS_INCLUDE_DIR}
    ${FUSE_TUTORIAL_DIR}/mnist_circ)

# dependencies on gzip: Bristol frontend
if (FUSE_BUILD_GZIP_HPP)
    set (FUSE_TEST_CPP
        ${FUSE_TEST_CPP}
        TestBristolFrontend.cpp)
    set (FUSE_TEST_INCLUDE_DIRS
        ${FUSE_TEST_INCLUDE_DIRS}
        ${FUSE_GZIP_INCLUDE_DIR})
    set (FUSE_TEST_LIBS
        ${FUSE_TEST_LIBS}
        ${MASON_PACKAGE_zlib_STATIC_LIBS})
endif(FUSE_BUILD_GZIP_HPP)

# dependencies on MOTION: tests for frontend and backend
if (FUSE_BUILD_MOTION_BACKEND OR FUSE_BUILD_MOTION_FRONTEND)
    set (FUSE_TEST_CPP
        ${FUSE_TEST_CPP}
        TestMOTIONFrontend.cpp
        TestMOTIONBackend.cpp)
    set (FUSE_TEST_LIBS
        ${FUSE_TEST_LIBS}
        FUSE_MOTION
        FUSE_MOTION_FE)
endif(FUSE_BUILD_MOTION_BACKEND OR FUSE_BUILD_MOTION_FRONTEND)

# dependencies on HyCC: tests for frontend
if (FUSE_BUILD_HyCC_FRONTEND)
    set (FUSE_TEST_CPP
        ${FUSE_TEST_CPP}
        TestDOTBackend.cpp
        TestHyCCFrontend.cpp)
    set (FUSE_TEST_LIBS
        ${FUSE_TEST_LIBS}
        FUSE_HyCC)
endif(FUSE_BUILD_HyCC_FRONTEND)

# dependencies on glasgow-subgraph-solver and distgraph: tests for subcircuit replacement pass
if (FUSE_BUILD_SUBGRAPH_OPTIMIZATION)
    set (FUSE_TEST_CPP
        ${FUSE_TEST_CPP}
        TestFrequentSubcircuitReplacement.cpp
        TestGraphBackend.cpp)
    set (FUSE_TEST_LIBS
        ${FUSE_TEST_LIBS}
        FUSE_DistGraph)
endif(FUSE_BUILD_SUBGRAPH_OPTIMIZATION)

###

########################### Define target etc. ###########################

add_executable(fusetest  ${FUSE_TEST_CPP})

target_link_libraries(fusetest PUBLIC ${FUSE_TEST_LIBS})

target_compile_features(fusetest PUBLIC cxx_std_20)

target_include_directories(fusetest PUBLIC ${FUSE_TEST_INCLUDE_DIRS})

set_target_properties(fusetest
        PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
        )

include(GoogleTest)
gtest_discover_tests(fusetest)
