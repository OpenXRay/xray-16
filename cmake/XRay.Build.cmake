include_guard()

set(CMAKE_CXX_STANDARD 17)
if (CMAKE_SYSTEM_NAME STREQUAL "Windows")
    set(CMAKE_CXX_STANDARD 20)
endif()
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Output all libraries and executables to one folder
set(XRAY_COMPILE_OUTPUT_FOLDER "${CMAKE_SOURCE_DIR}/bin/${CMAKE_SYSTEM_PROCESSOR}/$<CONFIG>")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${XRAY_COMPILE_OUTPUT_FOLDER}")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${XRAY_COMPILE_OUTPUT_FOLDER}")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${XRAY_COMPILE_OUTPUT_FOLDER}")
set(CMAKE_PDB_OUTPUT_DIRECTORY "${XRAY_COMPILE_OUTPUT_FOLDER}")
set(CMAKE_COMPILE_PDB_OUTPUT_DIRECTORY "${XRAY_COMPILE_OUTPUT_FOLDER}")

# Provide access to shared externals headers (e.g. submodule_check.hpp)
include_directories(
    "${CMAKE_SOURCE_DIR}/Externals"
    "${CMAKE_SOURCE_DIR}/Externals/luabind"
)

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
    set(CMAKE_INTERPROCEDURAL_OPTIMIZATION_RELEASE ON)
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

if (WIN32)
    include_directories("${XRAY_SDK_INCLUDE_DIR}")
    include_directories("${XRAY_SDK_INCLUDE_DIR}/SDL2")
    include_directories("${XRAY_SDK_INCLUDE_DIR}/nvapi")
    include_directories("${XRAY_SDK_INCLUDE_DIR}/AGS_SDK/ags_lib/inc")
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
