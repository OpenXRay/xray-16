cmake_minimum_required(VERSION 3.22)

message(STATUS "CMAKE_VERSION: ${CMAKE_VERSION}")

cmake_policy(SET CMP0048 NEW)
cmake_policy(SET CMP0095 NEW)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

include(utils)
calculate_xray_build_id(XRAY_BUILD_ID)

project(OpenXRay
    DESCRIPTION "OpenXRay is an improved version of the X-Ray Engine, \
                the game engine used in the world-famous S.T.A.L.K.E.R. game series by GSC Game World."
    VERSION 1.6.02.${XRAY_BUILD_ID}
    HOMEPAGE_URL "https://github.com/OpenXRay/xray-16"
    LANGUAGES CXX C
)

message(STATUS "CMAKE_PROJECT_VERSION: ${CMAKE_PROJECT_VERSION}")

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

option(CMAKE_VERBOSE_MAKEFILE "Verbose build output" OFF)
message(STATUS "CMAKE_VERBOSE_MAKEFILE: ${CMAKE_VERBOSE_MAKEFILE}")

if (CMAKE_VERBOSE_MAKEFILE)
    set(CMAKE_EXECUTE_PROCESS_COMMAND_ECHO STDOUT)
endif()

if (CMAKE_VERSION VERSION_EQUAL "3.28.2" AND CMAKE_UNITY_BUILD)
    # https://gitlab.kitware.com/cmake/cmake/-/issues/25650
    message(WARNING "In CMake 3.28.2, precompiled headers are broken when Unity build is enabled. This breaks project from compiling. \
                     Please, update to CMake 3.28.3 or downgrade to 3.28.1.")
    set(CMAKE_UNITY_BUILD OFF)
endif()

if (NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Release" CACHE STRING "" FORCE)
endif()

# Redirecting the default installation path /usr/local to /usr no need to use -DCMAKE_INSTALL_PREFIX =/usr
if (CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
    set(CMAKE_INSTALL_PREFIX "/usr")
endif()

include(GNUInstallDirs)
include(packaging)

set_git_info()

# Output all libraries and executable to one folder
set(COMPILE_OUTPUT_FOLDER "${CMAKE_SOURCE_DIR}/bin/${CMAKE_SYSTEM_PROCESSOR}/${CMAKE_BUILD_TYPE}")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${COMPILE_OUTPUT_FOLDER}")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${COMPILE_OUTPUT_FOLDER}")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${COMPILE_OUTPUT_FOLDER}")
set(CMAKE_PDB_OUTPUT_DIRECTORY "${COMPILE_OUTPUT_FOLDER}")
set(CMAKE_COMPILE_PDB_OUTPUT_DIRECTORY "${COMPILE_OUTPUT_FOLDER}")

if (DISABLE_PORTABLE_MODE)
    add_compile_definitions(DISABLE_PORTABLE_MODE)
endif()

set(CMAKE_BUILD_RPATH_USE_ORIGIN TRUE)
set(CMAKE_MACOSX_RPATH TRUE)

message(STATUS "CMAKE_SYSTEM_PROCESSOR: ${CMAKE_SYSTEM_PROCESSOR}")
if (CMAKE_SYSTEM_PROCESSOR STREQUAL "aarch64" OR CMAKE_SYSTEM_PROCESSOR STREQUAL "arm64")
    set(PROJECT_PLATFORM_ARM64 TRUE)
elseif (CMAKE_SYSTEM_PROCESSOR MATCHES "armv*")
    set(PROJECT_PLATFORM_ARM TRUE)
elseif (CMAKE_SYSTEM_PROCESSOR STREQUAL "e2k")
    set(PROJECT_PLATFORM_E2K TRUE)
elseif (CMAKE_SYSTEM_PROCESSOR STREQUAL "ppc" OR CMAKE_SYSTEM_PROCESSOR STREQUAL "ppc64le")
    set(PROJECT_PLATFORM_PPC TRUE)
endif()

if (CMAKE_BUILD_TYPE STREQUAL "Release" OR CMAKE_BUILD_TYPE STREQUAL "RelWithDebInfo")
    set(MASTER_GOLD_DEFAULT_VALUE ON)
else()
    set(MASTER_GOLD_DEFAULT_VALUE OFF)
endif()
message(STATUS "CMAKE_BUILD_TYPE: ${CMAKE_BUILD_TYPE}")

option(MASTER_GOLD "Build with MASTER_GOLD" ${MASTER_GOLD_DEFAULT_VALUE})
if (MASTER_GOLD)
    add_compile_definitions(MASTER_GOLD)
endif()
message(STATUS "MASTER_GOLD: ${MASTER_GOLD}")

option(STATIC_BUILD "Use static build" ${MASTER_GOLD})
if (STATIC_BUILD)
    # XXX: Uncomment only after build with XRAY_STATIC_BUILD is fixed
    #add_compile_definitions(XRAY_STATIC_BUILD)
endif()
message(STATUS "STATIC_BUILD: ${STATIC_BUILD}")

option(CMAKE_UNITY_BUILD "Use unity build" OFF)
message(STATUS "CMAKE_UNITY_BUILD: ${CMAKE_UNITY_BUILD}")

find_program(CCACHE_FOUND ccache)
if (CCACHE_FOUND)
    set_property(GLOBAL PROPERTY RULE_LAUNCH_COMPILE ccache)
    set_property(GLOBAL PROPERTY RULE_LAUNCH_LINK ccache)
    set(ENV{CCACHE_SLOPPINESS} pch_defines,time_macros)
endif ()

if (CMAKE_CXX_COMPILER_ID MATCHES "GNU")
    if (CMAKE_CXX_COMPILER_VERSION VERSION_LESS 8.0 AND NOT PROJECT_PLATFORM_E2K)
        message(FATAL_ERROR "Building with a gcc version less than 8.0 is not supported.")
    elseif (CMAKE_CXX_COMPILER_VERSION VERSION_LESS 7.0 AND PROJECT_PLATFORM_E2K)
        message(FATAL_ERROR "Building with a MCST lcc version less than 1.25 is not supported.")
    endif()
elseif (CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    # XXX: Remove -fdelayed-template-parsing
    add_compile_options(
        -fdelayed-template-parsing
        -Wno-unused-command-line-argument
        -Wno-inconsistent-missing-override
    )
endif()

if (CMAKE_CXX_COMPILER_ID MATCHES "Clang" AND NOT XRAY_USE_DEFAULT_CXX_LIB)
    if (NOT XRAY_CXX_LIB)
        include(CheckCXXCompilerFlag)
        CHECK_CXX_COMPILER_FLAG("-stdlib=libc++" LIBCPP_AVAILABLE)
        CHECK_CXX_COMPILER_FLAG("-stdlib=libstdc++" LIBSTDCPP_AVAILABLE)

        if (LIBCPP_AVAILABLE)
            set(XRAY_CXX_LIB "libc++" CACHE STRING "" FORCE)
        elseif (LIBSTDCPP_AVAILABLE)
            set(XRAY_CXX_LIB "libstdc++" CACHE STRING "" FORCE)
        else()
            message("Neither libstdc++ nor libc++ are available. Hopefully, system has another custom stdlib?")
        endif()
    endif()

    if (XRAY_CXX_LIB STREQUAL "libstdc++")
        add_compile_options(-stdlib=libstdc++)
    elseif (XRAY_CXX_LIB STREQUAL "libc++")
        add_compile_options(-stdlib=libc++)
        if (CMAKE_SYSTEM_NAME STREQUAL "FreeBSD")
            add_compile_options(-lcxxrt)
        else()
            add_compile_options(-lc++abi)
        endif()
    endif()
endif()

add_compile_options(-Wno-attributes)
if (APPLE)
    add_compile_options(-Wl,-undefined,error)
else()
    add_compile_options(-Wl,--no-undefined)
endif()

# TODO test
option(USE_ADDRESS_SANITIZER "Use AddressSanitizer" OFF)

if (USE_ADDRESS_SANITIZER)
    add_compile_options(
        -fsanitize=address
        -fsanitize=leak
        -fsanitize=undefined
        -fno-omit-frame-pointer
        -fno-optimize-sibling-calls
        -fno-sanitize=vptr
    )

    add_link_options(
        $<$<CXX_COMPILER_ID:Clang>:-shared-libasan>
    )
endif()

message(STATUS "USE_ADDRESS_SANITIZER: ${USE_ADDRESS_SANITIZER}")

option(USE_LTO "Use Link Time Optimization" ${MASTER_GOLD})
if (USE_LTO)
    include(CheckIPOSupported)
    check_ipo_supported(RESULT LTO_SUPPORTED)

    if (LTO_SUPPORTED)
        # With clang cmake only enables '-flto=thin' but we want full LTO
        if (CMAKE_CXX_COMPILER_ID MATCHES "Clang")
            add_compile_options(-flto=full)
        else()
            set(CMAKE_INTERPROCEDURAL_OPTIMIZATION ON)
        endif()
    endif()
endif()

message(STATUS "USE_LTO: ${USE_LTO}")

if (PROJECT_PLATFORM_ARM)
    add_compile_options(-mfpu=neon)
elseif (PROJECT_PLATFORM_ARM64)
    #add_compile_options()
elseif (PROJECT_PLATFORM_E2K)
    add_compile_options(-Wno-unknown-pragmas)
elseif (PROJECT_PLATFORM_PPC)
    add_compile_options(
        -maltivec
        -mabi=altivec
    )
    add_compile_definitions(NO_WARN_X86_INTRINSICS)
else()
    add_compile_options(
        -mfpmath=sse
        -msse3
    )
endif()

if (XRAY_LINKER)
    add_link_options(-fuse-ld=${XRAY_LINKER})
endif()

if (CMAKE_BUILD_TYPE STREQUAL "Debug")
    add_compile_definitions(
        DEBUG
        MIXED
    )
    add_compile_options(-Og)
endif()

add_compile_definitions(
    _MT
    _CPPUNWIND
)

if (NOT WIN32)
    find_package(SDL3 REQUIRED CONFIG REQUIRED COMPONENTS SDL3)
    if (NOT TARGET SDL3::SDL3 AND DEFINED SDL3_LIBRARIES)
        add_library(SDL3::SDL3 UNKNOWN IMPORTED)
        set_target_properties(
            SDL3::SDL3 PROPERTIES
            IMPORTED_LOCATION "${SDL3_LIBRARIES}"
            INTERFACE_INCLUDE_DIRECTORIES "${SDL3_INCLUDE_DIRS}"
        )
    endif()
    find_package(OpenGL REQUIRED)
    find_package(GLEW REQUIRED)
    find_package(OpenAL REQUIRED)
    find_package(JPEG)
    find_package(Ogg REQUIRED)
    find_package(Vorbis REQUIRED)
    find_package(Theora REQUIRED)
    find_package(LZO REQUIRED)
    find_package(mimalloc NAMES mimalloc2 mimalloc2.0 mimalloc)
endif()

# Memory allocator option
if (mimalloc_FOUND)
    set(MEMORY_ALLOCATOR "mimalloc" CACHE STRING "Use specific memory allocator (mimalloc/standard)")
else()
    set(MEMORY_ALLOCATOR "standard" CACHE STRING "Use specific memory allocator (mimalloc/standard)")
endif()
set_property(CACHE MEMORY_ALLOCATOR PROPERTY STRINGS "mimalloc" "standard")

if (MEMORY_ALLOCATOR STREQUAL "mimalloc" AND NOT mimalloc_FOUND)
    message(FATAL_ERROR "mimalloc allocator requested but not found. Please, install mimalloc package or select standard allocator.")
endif()

message("Using ${MEMORY_ALLOCATOR} memory allocator")

option(XRAY_USE_LUAJIT "Use LuaJIT" ON)

add_subdirectory(Externals)

add_compile_options(
    -Wall
    #-Werror
    -Wextra
    #-pedantic
    -Wno-unknown-pragmas
    -Wno-strict-aliasing
    -Wno-parentheses
    -Wno-unused-label
    -Wno-unused-parameter
    -Wno-switch
    #-Wno-padded
    #-Wno-c++98-compat
    #-Wno-c++98-compat-pedantic
    #-Wno-c++11-compat
    #-Wno-c++11-compat-pedantic
    #-Wno-c++14-compat
    #-Wno-c++14-compat-pedantic
    #-Wno-newline-eof
    $<$<CXX_COMPILER_ID:GNU>:$<$<COMPILE_LANGUAGE:CXX>:-Wno-class-memaccess>>
    $<$<CXX_COMPILER_ID:GNU>:$<$<COMPILE_LANGUAGE:CXX>:-Wno-interference-size>>
)

add_subdirectory(src)
add_subdirectory(res)
add_subdirectory(misc)

get_property(LIB64 GLOBAL PROPERTY FIND_LIBRARY_USE_LIB64_PATHS)

if ("${LIB64}" STREQUAL "TRUE")
    set(LIBSUFFIX 64)
else()
    set(LIBSUFFIX "")
endif()
