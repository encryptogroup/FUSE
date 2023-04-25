#
# MIT License
#
# Copyright (c) 2022 Nora Khayata, Moritz Huppert
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

if (FUSE_BUILD_MOTION_BACKEND)
    # Create Target for MOTION Backend
    add_library(FUSE_MOTION
        MOTIONBackend.h
        MOTIONBackend.cpp
    )

    # Set compiler options for Backend
    target_compile_features(FUSE_MOTION PRIVATE cxx_std_20)
    target_include_directories(FUSE_MOTION PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

    # Link Backend against FUSE and MOTION
    target_link_libraries(FUSE_MOTION PUBLIC FUSE)
    target_link_libraries(FUSE_MOTION PUBLIC motion)

    add_dependencies(FUSE_MOTION motion)

endif()

if (FUSE_BUILD_SUBGRAPH_OPTIMIZATION)
    message(STATUS "Add DistGraph libcircuit library")

    if (NOT EXISTS "${FUSE_DISTGRAPH_SRC_DIR}/Makefile")
        find_package(Git REQUIRED)
        message("initialize Git submodule: extern/DistGraph")
        execute_process(COMMAND git submodule update --init extern/DistGraph
                WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}")
    endif ()

    add_library(DistGraph_libseq STATIC IMPORTED)
    add_library(DistGraph_libglobals STATIC IMPORTED)
    add_library(DistGraph_pargraph_mpi_split_all STATIC IMPORTED)
    add_library(FUSE_DistGraph
        GraphBackend.cpp
        GraphBackend.h
        )

    if (NOT EXISTS "${FUSE_DISTGRAPH_SRC_DIR}/src/sequential/libseq.a")
        add_custom_target(DistGraph_generate_libseq make
                        BYPRODUCTS "${FUSE_DISTGRAPH_SRC_DIR}/src/sequential/libseq.a" "${FUSE_DISTGRAPH_SRC_DIR}/src/globals/libglobals.a" "${FUSE_DISTGRAPH_SRC_DIR}/src/parallel/paragraph_mpi_split_all"
                        WORKING_DIRECTORY ${FUSE_DISTGRAPH_SRC_DIR}
                        )
        add_dependencies(DistGraph_libseq PUBLIC DistGraph_generate_libseq DistGraph_pargraph_mpi_split_all)
    endif()

    set_target_properties(DistGraph_libseq PROPERTIES
        IMPORTED_LOCATION "${FUSE_DISTGRAPH_SRC_DIR}/src/sequential/libseq.a"
        INTERFACE_INCLUDE_DIRECTORIES ${FUSE_DISTGRAPH_SRC_DIR}/src/sequential
        )
    set_target_properties(DistGraph_libglobals PROPERTIES
        IMPORTED_LOCATION "${FUSE_DISTGRAPH_SRC_DIR}/src/globals/libglobals.a"
        INTERFACE_INCLUDE_DIRECTORIES "${FUSE_DISTGRAPH_SRC_DIR}/src/globals"
        )

    set_target_properties(DistGraph_pargraph_mpi_split_all PROPERTIES
        IMPORTED_LOCATION "${FUSE_DISTGRAPH_SRC_DIR}/src/parallel/pargraph_mpi_split_all"
        INTERFACE_INCLUDE_DIRECTORIES "${FUSE_DISTGRAPH_SRC_DIR}/src/parallel"
        INTERFACE_LINK_LIBRARIES DistGraph_libseq
        )

    target_compile_features(FUSE_DistGraph PRIVATE cxx_std_20)
    target_include_directories(FUSE_DistGraph PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

    target_link_libraries(FUSE_DistGraph PUBLIC FUSE)
    target_link_libraries(FUSE_DistGraph PRIVATE DistGraph_pargraph_mpi_split_all)
    target_link_libraries(FUSE_DistGraph PRIVATE DistGraph_libglobals)

    find_package(MPI REQUIRED)

    target_link_libraries(FUSE_DistGraph PRIVATE "${MPI_CXX_LIBRARIES}")
    include_directories(FUSE_DistGraph ${MPI_INCLUDE_PATH})

    message(STATUS "Add Glasgow library")

    if (NOT EXISTS "${FUSE_GLASGOW_SRC_DIR}/Makefile")
        find_package(Git REQUIRED)
        message("initialize Git submodule: extern/glasgow-subgraph-solver")
        execute_process(COMMAND git submodule update --init extern/glasgow-subgraph-solver
                WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}")
    endif ()

    add_library(Glasgow_lib STATIC IMPORTED)

    if (NOT EXISTS "${FUSE_GLASGOW_SRC_DIR}/libcommon.a")
        add_custom_target(Glasgow_generate_lib make
                        BYPRODUCTS "${FUSE_GLASGOW_SRC_DIR}/libcommon.a"
                        WORKING_DIRECTORY ${FUSE_GLASGOW_SRC_DIR}
                        )
        add_dependencies(Glasgow_lib PUBLIC Glasgow_generate_lib)
    endif()

    set_target_properties(Glasgow_lib PROPERTIES
        IMPORTED_LOCATION "${FUSE_GLASGOW_SRC_DIR}/libcommon.a"
        INTERFACE_INCLUDE_DIRECTORIES "${FUSE_GLASGOW_SRC_DIR}/src"
    )

    find_package(Boost REQUIRED COMPONENTS thread iostreams container)
    target_link_libraries(FUSE_DistGraph PRIVATE Boost::iostreams Boost::thread Boost::container)
    target_link_libraries(FUSE_DistGraph PRIVATE Glasgow_lib)
    include_directories(FUSE_DistGraph ${Boost_INCLUDE_DIRS})
endif()
