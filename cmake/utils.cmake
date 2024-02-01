function(target_sources_grouped)
    cmake_parse_arguments(
        PARSED_ARGS
        ""
        "TARGET;NAME;SCOPE"
        "FILES"
        ${ARGN}
    )

    if(NOT PARSED_ARGS_TARGET)
        message(FATAL_ERROR "You must provide a target name")
    endif()

    if(NOT PARSED_ARGS_NAME)
        message(FATAL_ERROR "You must provide a source group name")
    endif()

    if(NOT PARSED_ARGS_SCOPE)
        set(PARSED_ARGS_SCOPE PRIVATE)
    endif()

    target_sources(${PARSED_ARGS_TARGET} ${PARSED_ARGS_SCOPE} ${PARSED_ARGS_FILES})

    source_group(${PARSED_ARGS_NAME} FILES ${PARSED_ARGS_FILES})
endfunction()

# Detect arch type ( x86 or x64 )
if (CMAKE_SIZEOF_VOID_P EQUAL 8)
    set(ARCH_TYPE x64)
else (CMAKE_SIZEOF_VOID_P EQUAL 4)
    set(ARCH_TYPE x86)
endif()

# Unix system configuration
if (UNIX)
    # Try to find specific OS files to determine type of linux distribution
    find_file(FEDORA_FOUND fedora-release PATHS /etc)
    find_file(REDHAT_FOUND redhat-release inittab.RH PATHS /etc)
    find_file(CENTOS_FOUND centos-release PATHS /etc)
    # If we found debian then we don't need to check further for ubuntu
    # as it uses debian core.
    find_file(DEBIAN_FOUND debian_version debconf.conf PATHS /etc)

    # --------------------------------------------------
    # Uninstall target
    # --------------------------------------------------
    # To clean system folder from libraries and binaries
    # that was installed with `sudo make install`
    # just run `sudo make uninstall`
    if (NOT TARGET uninstall)
        configure_file(
                "${CMAKE_CURRENT_SOURCE_DIR}/cmake/cmake_uninstall.cmake.in"
                "${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake"
                IMMEDIATE @ONLY)

        add_custom_target(uninstall COMMAND ${CMAKE_COMMAND} -P ${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake)
    endif()
endif()

function(set_git_info)
    execute_process(COMMAND git rev-parse --verify HEAD
        WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
        OUTPUT_VARIABLE GIT_SHA1
        ERROR_QUIET
        OUTPUT_STRIP_TRAILING_WHITESPACE
    )
    message(STATUS "git commit: ${GIT_SHA1}")

    execute_process(COMMAND git rev-parse --abbrev-ref HEAD
        WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
        OUTPUT_VARIABLE GIT_BRANCH
        ERROR_QUIET
        OUTPUT_STRIP_TRAILING_WHITESPACE
    )
    message(STATUS "git branch: ${GIT_BRANCH}")
endfunction()

include(packaging)
