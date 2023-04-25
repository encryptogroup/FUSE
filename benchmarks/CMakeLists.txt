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

add_subdirectory(common)
add_subdirectory(file_size_comparisons)
add_subdirectory(loading_times_comparison)
add_subdirectory(ram_consumption)
add_subdirectory(motion_runtime_benchmarks)

################################################################################################

add_executable(generate_bm_candidates
                generate_candidates_main.cpp
                common/common.cpp
                )

target_compile_features(generate_bm_candidates PUBLIC cxx_std_20)

target_link_libraries(generate_bm_candidates PUBLIC
        FUSE
        FUSE_Bristol
        FUSE_HyCC
        FUSE_DistGraph
        fuse_bm_common
        flatbuffers
        ${MASON_PACKAGE_zlib_STATIC_LIBS}
        )

target_include_directories(generate_bm_candidates PUBLIC
        ${FUSE_FBS_INCLUDE_PREFIX}
        ${FUSE_FLATBUFFERS_INCLUDE_DIR}
        ${FUSE_GZIP_INCLUDE_DIR}
        ${FUSE_BENCHMARKS_DIR}
)

set_target_properties(generate_bm_candidates
        PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/bm"
)
