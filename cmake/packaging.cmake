set(PACKAGE_PROJECT_NAME        "OpenXRay")
set(PACKAGE_PROJECT_DESCRIPTION "OpenXRay is an improved version of the X-Ray Engine, the game engine used in the world-famous S.T.A.L.K.E.R. game series by GSC Game World.")
set(PACKAGE_PROJECT_CONTACT     "OpenXRay <openxray@yahoo.com>")
set(PACKAGE_PROJECT_VERSION     "1.6.02")
set(PACKAGE_PROJECT_HOME_URL    "https://github.com/OpenXRay/xray-16")

if (UNIX)
    if (EXISTS "${CMAKE_ROOT}/Modules/CPack.cmake")
        set(CPACK_PACKAGE_NAME ${PACKAGE_PROJECT_NAME})
        set(CPACK_FILE_NAME "openxray")
        set(CPACK_PACKAGE_VERSION ${PACKAGE_PROJECT_VERSION})
        set(CPACK_PACKAGE_CONTACT ${PACKAGE_PROJECT_CONTACT})
        set(CPACK_PACKAGE_DESCRIPTION ${PACKAGE_PROJECT_DESCRIPTION})
        set(CPACK_PACKAGE_DESCRIPTION_SUMMARY ${CPACK_PACKAGE_DESCRIPTION})
        set(CPACK_STRIP_FILES TRUE)
        set(CPACK_SOURCE_IGNORE_FILES "/.gitattributes")
        set(CPACK_RESOURCE_FILE_README ${PROJECT_SOURCE_DIR}/README.md)
        set(CPACK_RESOURCE_FILE_LICENSE ${PROJECT_SOURCE_DIR}/License.txt)

        # --- SELECT PROPER CPACK GENERATOR ---
        if (DEBIAN_FOUND)
            set(CPACK_GENERATOR DEB)

            set(CPACK_DEBIAN_FILE_NAME "DEB-DEFAULT")
            set(CPACK_DEBIAN_PACKAGE_CONTROL_STRICT_PERMISSION TRUE)
            set(CPACK_DEBIAN_REVISON "ubuntu-bionic-a1") # TODO: This one need to be detected dynamically
            set(CPACK_PACKAGE_FILE_NAME "${CPACK_PACKAGE_NAME}-${CPACK_PACKAGE_VERSION}-${CPACK_DEBIAN_REVISON}_${_OS_ARCH}")
            set(CPACK_DEBIAN_PACKAGE_SOURCE "OpenXRay")
            set(CPACK_DEBIAN_PACKAGE_SECTION "games")
            set(CPACK_DEBIAN_PACKAGE_HOMEPAGE ${PACKAGE_PROJECT_HOME_URL})
            set(CPACK_DEBIAN_PACKAGE_SHLIBDEPS ON)
        endif()

        if (FEDORA_FOUND OR REDHAT_FOUND OR CENTOS_FOUND)
            set(CPACK_GENERATOR RPM)

            # -- set(CPACK_RPM_PACKAGE_AUTOREQPROV ON)
            set(CPACK_RPM_PACKAGE_AUTOREQ ON)
            set(CPACK_RPM_PACKAGE_AUTOPROV YES)
            set(CPACK_RPM_PACKAGE_NAME ${PACKAGE_PROJECT_NAME})
            set(CPACK_RPM_PACKAGE_DESCRIPTION ${PACKAGE_PROJECT_DESCRIPTION})
            set(CPACK_RPM_PACKAGE_VERSION ${PACKAGE_PROJECT_VERSION})
            set(CPACK_RPM_PACKAGE_GROUP "Amusements/Games")
            set(CPACK_RPM_PACKAGE_VENDOR ${PACKAGE_PROJECT_NAME})
            set(CPACK_RPM_PACKAGE_URL ${PACKAGE_PROJECT_HOME_URL})
        endif()

        INCLUDE(CPack)
    endif()
endif()

# TODO: Need to be implemented in future
if (WIN32)
    #set(CPACK_GENERATOR NSIS)
endif()

if (APPLE)
    #set(CPACK_GENERATOR "DRAGNDROP")
endif()
