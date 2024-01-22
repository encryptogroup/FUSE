#
# MIT License
#
# Copyright (c) 2024 Nora Khayata
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

if (FUSE_BUILD_SUBGRAPH_OPTIMIZATION)
    add_executable(vec_pass vec_main.cpp)
    add_executable(sub_pass sub_repl_main.cpp)

    target_link_libraries(sub_pass PRIVATE
    FUSE
    FUSE_Bristol
    FUSE_DistGraph
    flatbuffers
    )

    target_include_directories(sub_pass PRIVATE
    ${FUSE_FBS_INCLUDE_PREFIX}
    ${FUSE_FLATBUFFERS_INCLUDE_DIR}
    )

    set_target_properties(sub_pass
    PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
    )

    target_link_libraries(vec_pass PRIVATE
            FUSE
            FUSE_Bristol
            ${MASON_PACKAGE_zlib_STATIC_LIBS}
    )

    set_target_properties(vec_pass
            PROPERTIES
            RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
    )
endif()

add_executable(call_analysis call_stack_analysis_main.cpp)

target_link_libraries(call_analysis PRIVATE
    FUSE
)

set_target_properties(call_analysis
PROPERTIES
RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)

add_executable(new_pass new_pass.cpp)

target_link_libraries(new_pass PRIVATE
    FUSE
)

set_target_properties(new_pass
PROPERTIES
RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)
