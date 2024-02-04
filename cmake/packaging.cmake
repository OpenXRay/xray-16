if (UNIX)
    if (EXISTS "${CMAKE_ROOT}/Modules/CPack.cmake")
        set(CPACK_PACKAGE_VENDOR "OpenXRay Team")
        set(CPACK_PACKAGE_CONTACT "OpenXRay <openxray@yahoo.com>")
        set(CPACK_PACKAGE_DESCRIPTION ${CMAKE_PROJECT_DESCRIPTION})

        set(CPACK_PACKAGE_FILE_NAME "openxray-${CMAKE_PROJECT_VERSION}-${CMAKE_SYSTEM_PROCESSOR}")

        set(CPACK_STRIP_FILES TRUE)
        set(CPACK_SOURCE_IGNORE_FILES "/.gitattributes")
        set(CPACK_RESOURCE_FILE_README ${PROJECT_SOURCE_DIR}/README.md)
        set(CPACK_RESOURCE_FILE_LICENSE ${PROJECT_SOURCE_DIR}/License.txt)

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
endif()

# TODO: Need to be implemented in future
if (WIN32)
    #set(CPACK_GENERATOR NSIS)
endif()

if (APPLE)
    #set(CPACK_GENERATOR "DRAGNDROP")
endif()
