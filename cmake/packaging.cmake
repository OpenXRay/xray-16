set(CPACK_PACKAGE_VENDOR "OpenXRay Team")
set(CPACK_PACKAGE_CONTACT "OpenXRay <openxray@yahoo.com>")
set(CPACK_PACKAGE_DESCRIPTION ${CMAKE_PROJECT_DESCRIPTION})

set(CPACK_PACKAGE_FILE_NAME "openxray-${CMAKE_PROJECT_VERSION}-${CMAKE_SYSTEM_PROCESSOR}")

set(CPACK_STRIP_FILES TRUE)
set(CPACK_SOURCE_IGNORE_FILES "/.gitattributes")
set(CPACK_RESOURCE_FILE_README ${PROJECT_SOURCE_DIR}/README.md)
set(CPACK_RESOURCE_FILE_LICENSE ${PROJECT_SOURCE_DIR}/License.txt)

if (UNIX)
    # Try to find specific OS files to determine type of linux distribution
    find_file(FEDORA_FOUND fedora-release PATHS /etc)
    find_file(REDHAT_FOUND redhat-release inittab.RH PATHS /etc)
    find_file(CENTOS_FOUND centos-release PATHS /etc)
    # If we found debian then we don't need to check further for ubuntu
    # as it uses debian core.
    find_file(DEBIAN_FOUND debian_version debconf.conf PATHS /etc)

    mark_as_advanced(FEDORA_FOUND REDHAT_FOUND CENTOS_FOUND DEBIAN_FOUND)

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

    # --- SELECT PROPER CPACK GENERATOR ---
    if (DEBIAN_FOUND)
        set(CPACK_GENERATOR DEB)

        set(CPACK_DEBIAN_FILE_NAME "DEB-DEFAULT")

        set(CPACK_DEBIAN_PACKAGE_SECTION "games")
        set(CPACK_DEBIAN_PACKAGE_SHLIBDEPS ON)
        set(CPACK_DEBIAN_PACKAGE_CONTROL_STRICT_PERMISSION TRUE)
    endif()

    if (FEDORA_FOUND OR REDHAT_FOUND OR CENTOS_FOUND)
        set(CPACK_GENERATOR RPM)

        set(CPACK_RPM_FILE_NAME "RPM-DEFAULT")

        set(CPACK_RPM_PACKAGE_GROUP "Amusements/Games")
        # -- set(CPACK_RPM_PACKAGE_AUTOREQPROV ON)
        set(CPACK_RPM_PACKAGE_AUTOREQ ON)
        set(CPACK_RPM_PACKAGE_AUTOPROV YES)
        set(CPACK_RPM_PACKAGE_RELEASE_DIST ON)
    endif()

    include(CPack)
endif()
