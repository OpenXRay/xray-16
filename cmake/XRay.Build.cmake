include_guard()

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

option(XRAY_UNITY_BUILD "Use unity (jumbo) builds for faster full compiles" ON)
if (XRAY_UNITY_BUILD AND NOT DEFINED CMAKE_UNITY_BUILD)
    set(CMAKE_UNITY_BUILD ON)
endif()

option(XRAY_USE_SCCACHE "Use sccache as compiler launcher when available" ON)
if (XRAY_USE_SCCACHE
    AND NOT CMAKE_C_COMPILER_LAUNCHER
    AND NOT CMAKE_CXX_COMPILER_LAUNCHER
    AND CMAKE_GENERATOR MATCHES "Ninja")
    find_program(SCCACHE_PROGRAM sccache)
    if (SCCACHE_PROGRAM)
        set(CMAKE_C_COMPILER_LAUNCHER   "${SCCACHE_PROGRAM}")
        set(CMAKE_CXX_COMPILER_LAUNCHER "${SCCACHE_PROGRAM}")
        if (MSVC)
            add_compile_options(/experimental:deterministic)
        endif()
        message(STATUS "sccache: ${SCCACHE_PROGRAM}")
    endif()
endif()

set(CMAKE_C_FLAGS_RELEASE   "-O2 -DNDEBUG" CACHE STRING "" FORCE)
set(CMAKE_CXX_FLAGS_RELEASE "-O2 -DNDEBUG" CACHE STRING "" FORCE)

set(CMAKE_C_FLAGS_RELEASEMASTERGOLD             "${CMAKE_C_FLAGS_RELEASE}"             CACHE STRING "" FORCE)
set(CMAKE_CXX_FLAGS_RELEASEMASTERGOLD           "${CMAKE_CXX_FLAGS_RELEASE}"           CACHE STRING "" FORCE)
set(CMAKE_EXE_LINKER_FLAGS_RELEASEMASTERGOLD    "${CMAKE_EXE_LINKER_FLAGS_RELEASE}"    CACHE STRING "" FORCE)
set(CMAKE_SHARED_LINKER_FLAGS_RELEASEMASTERGOLD "${CMAKE_SHARED_LINKER_FLAGS_RELEASE}" CACHE STRING "" FORCE)
set(CMAKE_STATIC_LINKER_FLAGS_RELEASEMASTERGOLD "${CMAKE_STATIC_LINKER_FLAGS_RELEASE}" CACHE STRING "" FORCE)
set(CMAKE_MODULE_LINKER_FLAGS_RELEASEMASTERGOLD "${CMAKE_MODULE_LINKER_FLAGS_RELEASE}" CACHE STRING "" FORCE)

set(CMAKE_C_FLAGS_MIXED                         "${CMAKE_C_FLAGS_DEBUG}"               CACHE STRING "" FORCE)
set(CMAKE_CXX_FLAGS_MIXED                       "${CMAKE_CXX_FLAGS_DEBUG}"             CACHE STRING "" FORCE)
set(CMAKE_EXE_LINKER_FLAGS_MIXED                "${CMAKE_EXE_LINKER_FLAGS_DEBUG}"      CACHE STRING "" FORCE)
set(CMAKE_SHARED_LINKER_FLAGS_MIXED             "${CMAKE_SHARED_LINKER_FLAGS_DEBUG}"   CACHE STRING "" FORCE)
set(CMAKE_STATIC_LINKER_FLAGS_MIXED             "${CMAKE_STATIC_LINKER_FLAGS_DEBUG}"   CACHE STRING "" FORCE)
set(CMAKE_MODULE_LINKER_FLAGS_MIXED             "${CMAKE_MODULE_LINKER_FLAGS_DEBUG}"   CACHE STRING "" FORCE)

# Output all libraries and executables to one folder
set(XRAY_COMPILE_OUTPUT_FOLDER "${CMAKE_SOURCE_DIR}/bin/${CMAKE_SYSTEM_PROCESSOR}/$<CONFIG>")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${XRAY_COMPILE_OUTPUT_FOLDER}")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${XRAY_COMPILE_OUTPUT_FOLDER}")
set(CMAKE_PDB_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/pdb/$<CONFIG>")
set(CMAKE_COMPILE_PDB_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/pdb/$<CONFIG>")

add_compile_definitions(
    # _DEBUG, DEBUG, MIXED, NDEBUG defines
    $<$<CONFIG:Debug>:_DEBUG>
    $<$<CONFIG:Debug,Mixed>:DEBUG>
    $<$<CONFIG:Mixed>:MIXED>
    $<$<CONFIG:Release,ReleaseMasterGold>:NDEBUG>
    # Tracy profiler
    $<$<BOOL:${XRAY_ENABLE_TRACY}>:TRACY_ENABLE>
    $<$<BOOL:${XRAY_ENABLE_TRACY}>:TRACY_NO_FRAME_IMAGE>
    # Luabind
    $<$<CONFIG:Release,ReleaseMasterGold>:LUABIND_NO_EXCEPTIONS>
    $<$<CONFIG:Release,ReleaseMasterGold>:LUABIND_NO_ERROR_CHECKING>
)

# Link-time optimization
include(CheckIPOSupported)
check_ipo_supported(RESULT LTO_IS_SUPPORTED)
if (LTO_IS_SUPPORTED)
    set(CMAKE_INTERPROCEDURAL_OPTIMIZATION_RELEASEMASTERGOLD ON)
endif()

# Main compiler settings
if (CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    include(XRay.Compiler.MSVC)
elseif (CMAKE_CXX_COMPILER_ID MATCHES "GNU|LCC|Clang")
    include(XRay.Compiler.GNULike)
else()
    message(FATAL_ERROR "Unsupported or unknown compiler.")
endif()

# https://gitlab.kitware.com/cmake/cmake/-/issues/25650
if (CMAKE_VERSION VERSION_EQUAL "3.28.2" AND CMAKE_UNITY_BUILD)
    message(WARNING
        "In CMake 3.28.2, precompiled headers are broken when Unity build is enabled. \
        We have to disable Unity build. Please, update to CMake 3.28.3 or downgrade to 3.28.1."
    )
    set(CMAKE_UNITY_BUILD OFF)
endif()

query_git_info(XRAY_GIT_SHA XRAY_GIT_BRANCH)

message(VERBOSE "CMAKE_UNITY_BUILD:     ${CMAKE_UNITY_BUILD}")
message(STATUS  "CMAKE_PROJECT_VERSION: ${CMAKE_PROJECT_VERSION}")
message(STATUS  "XRAY_GIT_SHA:          ${XRAY_GIT_SHA}")
message(STATUS  "XRAY_GIT_BRANCH:       ${XRAY_GIT_BRANCH}")

message(STATUS "BUILD_SHARED_LIBS:     ${BUILD_SHARED_LIBS}")
message(STATUS "LTO_IS_SUPPORTED:      ${LTO_IS_SUPPORTED}")

message(DEBUG)
message(DEBUG "C++ Flags:")
message(DEBUG "           Global: ${CMAKE_CXX_FLAGS}")
message(DEBUG "            Debug: ${CMAKE_CXX_FLAGS_DEBUG}")
message(DEBUG "            Mixed: ${CMAKE_CXX_FLAGS_MIXED}")
message(DEBUG "          Release: ${CMAKE_CXX_FLAGS_RELEASE}")
message(DEBUG "ReleaseMasterGold: ${CMAKE_CXX_FLAGS_RELEASEMASTERGOLD}")

message(DEBUG)
message(DEBUG "C Flags:")
message(DEBUG "           Global: ${CMAKE_C_FLAGS}")
message(DEBUG "            Debug: ${CMAKE_C_FLAGS_DEBUG}")
message(DEBUG "            Mixed: ${CMAKE_C_FLAGS_MIXED}")
message(DEBUG "          Release: ${CMAKE_C_FLAGS_RELEASE}")
message(DEBUG "ReleaseMasterGold: ${CMAKE_C_FLAGS_RELEASEMASTERGOLD}")
message(DEBUG)

unset(LTO_IS_SUPPORTED)
