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

add_executable(fusetest
        # TestWrappers.cpp
        # TestInterpreter.cpp
        #TestBristolFrontend.cpp
        # TestHyCCFrontend.cpp
        #TestDOTBackend.cpp
        # TestMOTIONBackend.cpp
        #TestDeadNodeOptimization.cpp
        #TestConstantFolding.cpp
        TestFrequentSubcircuitReplacement.cpp
        TestGraphBackend.cpp
        TestDepthAnalysis.cpp
        #TestLargeCircuits.cpp
        TestInstructionVectorization.cpp
        #TestMOTIONFrontend.cpp
        )


target_link_libraries(fusetest PUBLIC
        FUSE
        # FUSE_HyCC
        FUSE_Bristol
        #FUSE_MOTION
        #FUSE_MOTION_FE
        FUSE_DistGraph
        gtest_main
        flatbuffers
        ${MASON_PACKAGE_zlib_STATIC_LIBS}
        )

        target_compile_features(fusetest PUBLIC cxx_std_20)

target_include_directories(fusetest PUBLIC
        ${FUSE_FBS_INCLUDE_PREFIX}
        ${FUSE_FLATBUFFERS_INCLUDE_DIR}
        ${FUSE_GZIP_INCLUDE_DIR})

set_target_properties(fusetest
        PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
        )

include(GoogleTest)
gtest_discover_tests(fusetest)
