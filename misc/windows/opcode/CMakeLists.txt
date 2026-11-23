cmake_minimum_required(VERSION 3.23)
project(OpenXRayOpcode13Checks LANGUAGES CXX C)

# Build only OPCODE/xrCDB and their checks, importing unchanged native engine
# dependencies. This avoids another complete engine/SDK build for the update.
if (NOT MSVC OR NOT CMAKE_SIZEOF_VOID_P EQUAL 8)
    message(FATAL_ERROR "This project requires MSVC x64.")
endif()
get_filename_component(XRAY_SOURCE_DIR "${CMAKE_CURRENT_LIST_DIR}/../../.." ABSOLUTE)
set(XRAY_NATIVE_BUILD_ROOT "${XRAY_SOURCE_DIR}" CACHE PATH "Matching native engine checkout")
set(XRAY_NATIVE_CONFIGURATION Release CACHE STRING "Native engine configuration")
set(CMAKE_CONFIGURATION_TYPES "${XRAY_NATIVE_CONFIGURATION}" CACHE STRING "Configuration" FORCE)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
set(BUILD_SHARED_LIBS ON)
set(XRAY_BUILD_CDB_TESTS ON)
list(APPEND CMAKE_MODULE_PATH "${XRAY_SOURCE_DIR}/cmake")
include(utils)
include(GNUInstallDirs)
enable_testing()

add_compile_options(/EHsc /fp:precise /wd4201 /wd4251 /wd4275 /wd4458)
add_compile_definitions(MSVC WIN32 XRAY_EXCEPTIONS=1 LUABIND_DYNAMIC_LINK
    $<$<CONFIG:Debug>:DEBUG;_DEBUG> $<$<CONFIG:Release>:NDEBUG>)
file(STRINGS "${XRAY_SOURCE_DIR}/src/xrCDB/packages.config" sdl_package REGEX "id=\"sdl2.nuget\"")
string(REGEX MATCH "version=\"([^\"]+)\"" sdl_version "${sdl_package}")
set(sdl_include "${XRAY_NATIVE_BUILD_ROOT}/src/packages/sdl2.nuget.${CMAKE_MATCH_1}/build/native/include")
if (NOT EXISTS "${sdl_include}/SDL.h")
    message(FATAL_ERROR "Restore matching SDL NuGet packages in XRAY_NATIVE_BUILD_ROOT.")
endif()
include_directories("${XRAY_SOURCE_DIR}/src" "${XRAY_SOURCE_DIR}/sdk/include"
    "${XRAY_SOURCE_DIR}/Externals" "${XRAY_SOURCE_DIR}/Externals/tracy/public" "${sdl_include}"
    "${XRAY_NATIVE_BUILD_ROOT}/Externals/luabind"
    "${XRAY_NATIVE_BUILD_ROOT}/Externals/LuaJIT/src")
set(native_lib "${XRAY_NATIVE_BUILD_ROOT}/build/lib/x64/${XRAY_NATIVE_CONFIGURATION}")
set(native_bin "${XRAY_NATIVE_BUILD_ROOT}/bin/x64/${XRAY_NATIVE_CONFIGURATION}")
foreach(name xrCore xrAPI xrEngine)
    add_library(${name} SHARED IMPORTED GLOBAL)
    set_target_properties(${name} PROPERTIES IMPORTED_IMPLIB "${native_lib}/${name}.lib"
        IMPORTED_LOCATION "${native_bin}/${name}.dll")
endforeach()
add_library(xrMiscMath STATIC IMPORTED GLOBAL)
set_target_properties(xrMiscMath PROPERTIES IMPORTED_LOCATION "${native_lib}/xrMiscMath.lib")
add_subdirectory("${XRAY_SOURCE_DIR}/Externals/OPCODE" opcode)
add_subdirectory("${XRAY_SOURCE_DIR}/src/xrCDB" cdb)
target_link_libraries(xrCDB PRIVATE xrEngine)
