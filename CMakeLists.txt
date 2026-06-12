cmake_minimum_required(VERSION 3.23)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
include(PreProjectInit)

calculate_xray_build_id(XRAY_BUILD_ID)
project(OpenXRay
    DESCRIPTION "OpenXRay is an improved version of the X-Ray Engine, \
                the game engine used in the world-famous S.T.A.L.K.E.R. game series by GSC Game World."
    VERSION 1.6.02.${XRAY_BUILD_ID}
    HOMEPAGE_URL "https://github.com/OpenXRay/xray-16"
    LANGUAGES CXX C
)

if (APPLE)
    enable_language(OBJC)
    enable_language(OBJCXX)
endif()

option(BUILD_SHARED_LIBS "Build shared libraries"     ${BUILD_SHARED_LIBS_DEFAULT_VALUE})

option(XRAY_USE_LUAJIT   "Use LuaJIT"                 ON)
option(XRAY_USE_ASAN     "Use AddressSanitizer"       OFF)
option(XRAY_ENABLE_TRACY "Enable tracy profiler"      OFF)
option(XRAY_USE_AI_PBR   "Enable AI PBR texture converter (requires ONNX Runtime)" OFF)

include(XRay.Build)
include(XRay.Windows.Externals)
include(XRay.Packaging)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if (CMAKE_GENERATOR MATCHES "Visual Studio")
    set_property(GLOBAL PROPERTY USE_FOLDERS ON)
    set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT xr_3da)
endif()

block()
    set(CMAKE_UNITY_BUILD OFF)
    add_subdirectory(Externals)
endblock()
add_subdirectory(src)
add_subdirectory(res)
add_subdirectory(misc)

if(MSVC)
    function(xray_set_per_target_pdb dir)
        get_property(_subdirs DIRECTORY ${dir} PROPERTY SUBDIRECTORIES)
        foreach(_sub IN LISTS _subdirs)
            xray_set_per_target_pdb(${_sub})
        endforeach()
        get_property(_targets DIRECTORY ${dir} PROPERTY BUILDSYSTEM_TARGETS)
        foreach(_tgt IN LISTS _targets)
            get_target_property(_type ${_tgt} TYPE)
            if(NOT _type STREQUAL "INTERFACE_LIBRARY" AND NOT _type STREQUAL "UTILITY")
                set_target_properties(${_tgt} PROPERTIES
                    COMPILE_PDB_NAME ${_tgt}
                    PDB_NAME ${_tgt})
            endif()
        endforeach()
    endfunction()
    xray_set_per_target_pdb(${CMAKE_SOURCE_DIR})
endif()

set(XRAY_GAME_DATA_DIR "" CACHE PATH
    "Working directory used when launching xr_3da from the IDE (must contain fsgame.ltx). Leave blank to inherit MSBuild default.")

if (CMAKE_GENERATOR MATCHES "Visual Studio" AND TARGET xr_3da AND XRAY_GAME_DATA_DIR)
    set_target_properties(xr_3da PROPERTIES
        VS_DEBUGGER_WORKING_DIRECTORY "${XRAY_GAME_DATA_DIR}"
        VS_DEBUGGER_COMMAND "$<TARGET_FILE:xr_3da>"
    )
endif()
