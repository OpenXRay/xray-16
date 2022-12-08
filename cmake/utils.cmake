# XXX: remove add_dir macro after Externals/GameSpy cmake refactoring
macro(add_dir DIRS)
    foreach (dir ${DIRS})
        message("adding  ${dir} to ${PROJECT_NAME}")
        include_directories(${dir})
        file(GLOB ${dir}__INCLUDES_H ${dir} ${dir}/*.h)
        file(GLOB ${dir}__INCLUDES_HPP ${dir} ${dir}/*.hpp)
        list(APPEND ${PROJECT_NAME}__INCLUDES ${${dir}__INCLUDES_H} ${${dir}__INCLUDES_HPP})
        file(GLOB ${dir}__SOURCES_CPP ${dir} ${dir}/*.cpp ${dir}/*.cxx)
        file(GLOB ${dir}__SOURCES_C ${dir} ${dir}/*.c)
        list(APPEND ${PROJECT_NAME}__SOURCES ${${dir}__SOURCES_C} ${${dir}__SOURCES_CPP})
    endforeach()
endmacro()

macro(group_sources SRC_FILES)
    foreach(source IN LISTS SRC_FILES)
        get_filename_component(source_path "${source}" PATH)
        string(REPLACE "/" "\\" source_path_msvc "${source_path}")
        source_group("${source_path_msvc}" FILES "${source}")
    endforeach()
endmacro()

# ------------------------------------------
# Detect arch type ( x86 or x64 )
# ------------------------------------------
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

include(${PROJECT_SOURCE_DIR}/cmake/packaging.cmake)
