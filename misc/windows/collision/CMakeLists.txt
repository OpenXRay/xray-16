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
option(XRAY_BUILD_CDB_TESTS "Build collision query regression tests" OFF)
option(XRAY_USE_JOLT_CDB "Use the shared Jolt collision and physics mesh" ON)
option(XRAY_BUILD_PHYSICS_TESTS "Build physics backend regression tests" OFF)
option(XRAY_BUILD_PHYSICS_BENCHMARKS "Build comparable complete-step physics benchmarks" OFF)
option(XRAY_GAMEPLAY_BENCHMARK "Enable opt-in in-game benchmark capture" OFF)
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
    "${sdl_include}")

set(XRAY_NATIVE_BUILD_ROOT "${XRAY_SOURCE_DIR}" CACHE PATH "Checkout containing matching native engine libraries")
set(native_lib "${XRAY_NATIVE_BUILD_ROOT}/build/lib/x64/${XRAY_NATIVE_CONFIGURATION}")
set(native_bin "${XRAY_NATIVE_BUILD_ROOT}/bin/x64/${XRAY_NATIVE_CONFIGURATION}")
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
add_subdirectory("${XRAY_SOURCE_DIR}/Externals/Jolt-proj" jolt)
if (NOT XRAY_USE_JOLT_CDB)
    import_native(xrOPCODE OPCODE SHARED)
endif()
add_subdirectory("${XRAY_SOURCE_DIR}/src/xrCDB" cdb)
add_subdirectory("${XRAY_SOURCE_DIR}/src/xrPhysicsCore" physics-core)
add_subdirectory("${XRAY_SOURCE_DIR}/src/xrPhysics" physics)
if (XRAY_BUILD_PHYSICS_BENCHMARKS)
    import_native(xrODEBenchmark ODE SHARED)
    add_executable(xrODEPhysicsBenchmark "${XRAY_SOURCE_DIR}/src/xrPhysicsCore/tests/PhysicsSceneBenchmark.cpp")
    target_compile_definitions(xrODEPhysicsBenchmark PRIVATE XRAY_BENCHMARK_ODE)
    target_include_directories(xrODEPhysicsBenchmark PRIVATE "${XRAY_SOURCE_DIR}/Externals/ode/include")
    target_link_libraries(xrODEPhysicsBenchmark PRIVATE xrODEBenchmark xrCore Psapi)
    target_link_options(xrODEPhysicsBenchmark PRIVATE /STACK:67108864)
    add_custom_command(TARGET xrODEPhysicsBenchmark POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${native_bin}/ODE.dll" "$<TARGET_FILE_DIR:xrODEPhysicsBenchmark>/ODE.dll")
endif()
option(XRAY_BUILD_NATIVE_GAME "Rebuild the game against native physics interfaces" ON)
if (XRAY_BUILD_NATIVE_GAME)
    foreach(name xrAICore xrUICore xrGameSpy)
        import_native(${name} ${name} SHARED)
    endforeach()
    import_native(xrNetServer xrNetServer SHARED)
    import_native(xrImGui imgui STATIC)
    import_native(GameSpy GameSpy STATIC)
    add_subdirectory("${XRAY_SOURCE_DIR}/src/xrGame" game)
    target_sources(xrGame PRIVATE "${XRAY_SOURCE_DIR}/src/xrGame/AnselManager.cpp")
    target_link_libraries(xrGame PRIVATE GameSpy "${XRAY_NATIVE_BUILD_ROOT}/sdk/libraries/x64/AnselSDK64.lib" delayimp)
    target_link_options(xrGame PRIVATE /DELAYLOAD:AnselSDK64.dll)
endif()
