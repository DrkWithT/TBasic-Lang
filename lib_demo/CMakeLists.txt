if (DEFINED CREATE_TBASIC_SHARED)
    add_executable(tbasic_demo)
    target_include_directories(tbasic_demo PUBLIC ${PROJECT_INCLUDES})
    # NOTE: The tbasic_lib should be installed at the project directory root by this point.
    target_link_directories(tbasic_demo PRIVATE "${PROJECT_SHARED_LIB_OUTPUT_DIR}")
    target_sources(tbasic_demo PUBLIC "${PROJECT_ROOT}/lib_demo/hello_demo.c")
    target_link_libraries(tbasic_demo PRIVATE tbasic_lib)

    add_executable(tbasic_invoke_demo)
    target_include_directories(tbasic_invoke_demo PUBLIC ${PROJECT_INCLUDES})
    # NOTE: The tbasic_lib should be installed at the project directory root by this point.
    target_link_directories(tbasic_invoke_demo PRIVATE "${PROJECT_SHARED_LIB_OUTPUT_DIR}")
    target_sources(tbasic_invoke_demo PUBLIC "${PROJECT_ROOT}/lib_demo/invoke_demo.c")
    target_link_libraries(tbasic_invoke_demo PRIVATE tbasic_lib)

    message(NOTICE "Completed. See ./build directory for the 'tbasic_lib' shared library.")
else ()
    message(FATAL_ERROR "The top-level CMakeLists.txt must run to generate TBasic's shared library before attempting to build the demo.")
endif ()
