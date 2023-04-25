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

add_executable(motion_vectorization_bm
motion_implementation.hpp
motion_vectorization_bm.h
motion_bm_main.cpp
../common/common.cpp
                )

if (NOT MOTION_BUILD_BOOST_FROM_SOURCES)
    find_package(Boost
            COMPONENTS
            program_options
            REQUIRED)
endif ()

target_compile_features(motion_vectorization_bm PRIVATE cxx_std_20)

target_link_libraries(motion_vectorization_bm PUBLIC
        Boost::program_options
        FUSE
        FUSE_MOTION
        FUSE_MOTION_FE
        FUSE_Bristol
        FUSE_HyCC
        FUSE_DistGraph
        fuse_bm_common
        flatbuffers
        MOTION::motion
        ${MASON_PACKAGE_zlib_STATIC_LIBS}
        )

target_include_directories(motion_vectorization_bm PUBLIC
        ${FUSE_FBS_INCLUDE_PREFIX}
        ${FUSE_FLATBUFFERS_INCLUDE_DIR}
        ${FUSE_GZIP_INCLUDE_DIR}
        ${FUSE_BENCHMARKS_DIR}
)


set_target_properties(motion_vectorization_bm
        PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/bm"
)
