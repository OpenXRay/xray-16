cmake_minimum_required(VERSION 3.23)
project(OpenXRayWindowsCollisionTests LANGUAGES CXX C)

# A bridge to the supported native Windows build, until the top-level CMake
# project supports MSVC. Build the real collision/physics targets and import
# the remaining engine DLLs from the same checkout and configuration.
if (NOT MSVC OR NOT CMAKE_SIZEOF_VOID_P EQUAL 8)
    message(FATAL_ERROR "This project requires MSVC x64.")
endif()

get_filename_component(XRAY_SOURCE_DIR "${CMAKE_CURRENT_LIST_DIR}/../../.." ABSOLUTE)
set(XRAY_NATIVE_CONFIGURATION Debug CACHE STRING "Configuration of the native engine build")
if (NOT XRAY_NATIVE_CONFIGURATION MATCHES "^(Debug|Release)$")
    message(FATAL_ERROR "XRAY_NATIVE_CONFIGURATION must be Debug or Release.")
endif()
set(CMAKE_CONFIGURATION_TYPES "${XRAY_NATIVE_CONFIGURATION}" CACHE STRING "Build configuration" FORCE)
if (NOT CMAKE_CONFIGURATION_TYPES OR NOT CMAKE_GENERATOR MATCHES "Visual Studio")
    message(FATAL_ERROR "Use a Visual Studio generator matching the native build.")
endif()

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
set(BUILD_SHARED_LIBS ON)
set(XRAY_BUILD_CDB_TESTS ON)
option(XRAY_USE_JOLT_CDB "Use Jolt instead of OPCODE" OFF)
list(APPEND CMAKE_MODULE_PATH "${XRAY_SOURCE_DIR}/cmake")
include(utils)
include(GNUInstallDirs)
enable_testing()

# Match the native non-Master-Gold ABI (src/xray_build.props). In particular,
# Debug uses the debug CRT/STL and Release uses the release CRT/STL.
add_compile_options(/EHsc /fp:precise /wd4201 /wd4251 /wd4275 /wd4458)
add_compile_definitions(MSVC WIN32 dSINGLE XRAY_EXCEPTIONS=1 LUABIND_DYNAMIC_LINK
    $<$<CONFIG:Debug>:DEBUG;_DEBUG>
    $<$<CONFIG:Release>:NDEBUG>)
file(STRINGS "${XRAY_SOURCE_DIR}/src/xrCDB/packages.config" sdl_package REGEX "id=\"sdl2.nuget\"")
string(REGEX MATCH "version=\"([^\"]+)\"" sdl_version "${sdl_package}")
set(sdl_include "${XRAY_SOURCE_DIR}/src/packages/sdl2.nuget.${CMAKE_MATCH_1}/build/native/include")
if (NOT EXISTS "${sdl_include}/SDL.h")
    message(FATAL_ERROR "Restore src/engine.sln with NuGet before configuring.")
endif()
include_directories(
    "${XRAY_SOURCE_DIR}/src"
    "${XRAY_SOURCE_DIR}/sdk/include"
    "${XRAY_SOURCE_DIR}/Externals"
    "${XRAY_SOURCE_DIR}/Externals/LuaJIT/src"
    "${XRAY_SOURCE_DIR}/Externals/luabind"
    "${XRAY_SOURCE_DIR}/Externals/imgui"
    "${XRAY_SOURCE_DIR}/Externals/tracy/public"
    "${XRAY_SOURCE_DIR}/Externals/ode/include"
    "${sdl_include}")

set(native_lib "${XRAY_SOURCE_DIR}/build/lib/x64/${XRAY_NATIVE_CONFIGURATION}")
set(native_bin "${XRAY_SOURCE_DIR}/bin/x64/${XRAY_NATIVE_CONFIGURATION}")
function(import_native target filename type)
    if (NOT EXISTS "${native_lib}/${filename}.lib")
        message(FATAL_ERROR "Missing ${native_lib}/${filename}.lib; build the native solution first.")
    endif()
    add_library(${target} ${type} IMPORTED GLOBAL)
    if (type STREQUAL "SHARED")
        set_target_properties(${target} PROPERTIES
            IMPORTED_IMPLIB "${native_lib}/${filename}.lib"
            IMPORTED_LOCATION "${native_bin}/${filename}.dll")
    else()
        set_target_properties(${target} PROPERTIES IMPORTED_LOCATION "${native_lib}/${filename}.lib")
    endif()
endfunction()

foreach(name xrCore xrAPI xrSound xrEngine xrMaterialSystem xrScriptEngine luabind LuaJIT)
    import_native(${name} ${name} SHARED)
endforeach()
set_property(TARGET xrScriptEngine PROPERTY INTERFACE_LINK_LIBRARIES "luabind;LuaJIT")
import_native(xrMiscMath xrMiscMath STATIC)
import_native(xrODE ODE SHARED)
if (XRAY_USE_JOLT_CDB)
    add_subdirectory("${XRAY_SOURCE_DIR}/Externals/Jolt-proj" jolt)
else()
    import_native(xrOPCODE OPCODE SHARED)
endif()
add_subdirectory("${XRAY_SOURCE_DIR}/src/xrCDB" cdb)
add_subdirectory("${XRAY_SOURCE_DIR}/src/xrPhysics" physics)
