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

add_executable(fuse_ram_bm
                fuse_ram_bm.cpp
                ../common/common.cpp
                )

add_executable(hycc_ram_bm
                hycc_ram_bm.cpp
                ../common/common.cpp
                )

target_compile_features(fuse_ram_bm PRIVATE cxx_std_20)

target_compile_features(hycc_ram_bm PRIVATE cxx_std_20)

target_link_libraries(fuse_ram_bm PUBLIC
        FUSE
        FUSE_Bristol
        FUSE_HyCC
        flatbuffers
        fuse_bm_common
        ${MASON_PACKAGE_zlib_STATIC_LIBS}
        )

target_link_libraries(hycc_ram_bm PUBLIC
        FUSE
        FUSE_Bristol
        FUSE_HyCC
        flatbuffers
        fuse_bm_common
        ${MASON_PACKAGE_zlib_STATIC_LIBS}
        )

target_include_directories(fuse_ram_bm PUBLIC
        ${FUSE_FBS_INCLUDE_PREFIX}
        ${FUSE_FLATBUFFERS_INCLUDE_DIR}
        ${FUSE_GZIP_INCLUDE_DIR}
        ${FUSE_BENCHMARKS_DIR}
)

target_include_directories(hycc_ram_bm PUBLIC
        ${FUSE_FBS_INCLUDE_PREFIX}
        ${FUSE_FLATBUFFERS_INCLUDE_DIR}
        ${FUSE_GZIP_INCLUDE_DIR}
        ${FUSE_BENCHMARKS_DIR}
)

set_target_properties(fuse_ram_bm
        PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/bm"
)

set_target_properties(hycc_ram_bm
        PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/bm"
)
