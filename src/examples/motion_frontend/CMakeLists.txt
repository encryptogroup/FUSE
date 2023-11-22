if (FUSE_BUILD_MOTION_FRONTEND)
    add_executable(motion_frontend
                    motion_frontend_main.cpp)

    if (NOT MOTION_BUILD_BOOST_FROM_SOURCES)
        find_package(Boost
                COMPONENTS
                program_options
                REQUIRED)
    endif ()

    target_compile_features(motion_frontend PRIVATE cxx_std_20)

    target_link_libraries(motion_frontend PUBLIC
            Boost::program_options
            FUSE
            FUSE_Bristol
            FUSE_MOTION
            FUSE_MOTION_FE
            flatbuffers
            )

    target_include_directories(motion_frontend PUBLIC
            ${FUSE_FBS_INCLUDE_PREFIX}
            ${FUSE_FLATBUFFERS_INCLUDE_DIR}
    )


    set_target_properties(motion_frontend
            PROPERTIES
            RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
    )
endif()
