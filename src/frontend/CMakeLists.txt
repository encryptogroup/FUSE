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

if (FUSE_BUILD_HyCC_FRONTEND)
    message(STATUS "Add HyCC libcircuit library")

    if (NOT EXISTS "${FUSE_HyCC_SRC_DIR}/Makefile")
        find_package(Git REQUIRED)
        message("initialize Git submodule: extern/HyCC")
        execute_process(COMMAND git submodule update --init extern/HyCC
                WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}")
    endif ()

    # add_custom_command(OUTPUT "${FUSE_HyCC_SRC_DIR}/bin/libcircuit.a"
    #                COMMAND make minisat2-download
    #                COMMAND make
    #                WORKING_DIRECTORY ${FUSE_HyCC_SRC_DIR}
    # )

    add_library(HyCC_libcircuit STATIC IMPORTED)
    add_library(FUSE_HyCC
        HyCCFrontend.cpp
        )

    if (NOT EXISTS "${FUSE_HyCC_SRC_DIR}/src/libcircuit/libcircuit.a")
        add_custom_target(HyCC_generate_libcircuit make minisat2-download
                        COMMAND make
                        BYPRODUCTS "${FUSE_HyCC_SRC_DIR}/src/libcircuit/libcircuit.a"
                        WORKING_DIRECTORY ${FUSE_HyCC_SRC_DIR}
                        )
        add_dependencies(HyCC_libcircuit PUBLIC HyCC_generate_libcircuit)
    endif()

    set_target_properties(HyCC_libcircuit PROPERTIES
        IMPORTED_LOCATION "${FUSE_HyCC_SRC_DIR}/src/libcircuit/libcircuit.a"
        INTERFACE_INCLUDE_DIRECTORIES ${FUSE_HyCC_SRC_DIR}/src
        )


    target_compile_features(FUSE_HyCC PRIVATE cxx_std_20)
    target_include_directories(FUSE_HyCC PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

    target_link_libraries(FUSE_HyCC PUBLIC FUSE)
    target_link_libraries(FUSE_HyCC PUBLIC HyCC_libcircuit)
endif()

if (FUSE_BUILD_BRISTOL_FRONTEND)
    add_library(FUSE_Bristol
    BristolFrontend.cpp
    )
    target_compile_features(FUSE_Bristol PRIVATE cxx_std_20)
    target_include_directories(FUSE_Bristol PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

    target_link_libraries(FUSE_Bristol PUBLIC FUSE)
endif()

if (FUSE_BUILD_MOTION_FRONTEND)
    # create target for MOTION frontend
    add_library(FUSE_MOTION_FE
        MOTIONFrontend.h
        MOTIONFrontend.cpp
        )

    # set compiler options for frontend
    target_compile_features(FUSE_MOTION_FE PRIVATE cxx_std_20)
    target_include_directories(FUSE_MOTION_FE PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

    # link frontend against FUSE and MOTION
    target_link_libraries(FUSE_MOTION_FE PUBLIC FUSE)
    target_link_libraries(FUSE_MOTION_FE PUBLIC motion)

    add_dependencies(FUSE_MOTION_FE motion)
endif()
