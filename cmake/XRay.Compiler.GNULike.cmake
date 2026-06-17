include_guard()

if (APPLE)
    if (NOT CMAKE_OSX_DEPLOYMENT_TARGET)
        if (DEFINED ENV{MACOSX_DEPLOYMENT_TARGET} AND NOT "$ENV{MACOSX_DEPLOYMENT_TARGET}" STREQUAL "")
            set(CMAKE_OSX_DEPLOYMENT_TARGET $ENV{MACOSX_DEPLOYMENT_TARGET})
        else()
            # CMAKE_SYSTEM_VERSION is the Darwin kernel version (e.g. 25.4.0), not the
            # macOS product version clang expects for MACOSX_DEPLOYMENT_TARGET.
            execute_process(
                COMMAND sw_vers -productVersion
                OUTPUT_VARIABLE _MACOS_PRODUCT_VERSION
                OUTPUT_STRIP_TRAILING_WHITESPACE
            )
            message(NOTICE "CMAKE_OSX_DEPLOYMENT_TARGET is not set, defaulting it to your system's macOS version: ${_MACOS_PRODUCT_VERSION}")
            set(CMAKE_OSX_DEPLOYMENT_TARGET ${_MACOS_PRODUCT_VERSION})
        endif()
    endif()
    message(STATUS "CMAKE_OSX_DEPLOYMENT_TARGET: ${CMAKE_OSX_DEPLOYMENT_TARGET}")
endif()

# Redirecting the default installation path /usr/local to /usr no need to use -DCMAKE_INSTALL_PREFIX =/usr
if (CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
    set(CMAKE_INSTALL_PREFIX "/usr")
endif()

include(GNUInstallDirs)

if (DISABLE_PORTABLE_MODE)
    add_compile_definitions(DISABLE_PORTABLE_MODE)
endif()

set(CMAKE_BUILD_RPATH_USE_ORIGIN TRUE)
set(CMAKE_MACOSX_RPATH TRUE)

find_program(CCACHE_FOUND ccache)
if (CCACHE_FOUND)
    set_property(GLOBAL PROPERTY RULE_LAUNCH_COMPILE ccache)
    set_property(GLOBAL PROPERTY RULE_LAUNCH_LINK ccache)
    set(ENV{CCACHE_SLOPPINESS} pch_defines,time_macros)
endif ()

if (CMAKE_CXX_COMPILER_ID MATCHES "GNU")
    if (CMAKE_CXX_COMPILER_VERSION VERSION_LESS 8.0)
        message(FATAL_ERROR "Building with a gcc version less than 8.0 is not supported.")
    endif()
elseif (CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    # XXX: Remove -fdelayed-template-parsing
    add_compile_options(
        -fdelayed-template-parsing
        -Wno-unused-command-line-argument
        -Wno-inconsistent-missing-override
    )
endif()

if (WIN32)
    add_compile_definitions(WIN32 _WINDOWS)
endif()

if (CMAKE_CXX_COMPILER_ID MATCHES "Clang" AND NOT XRAY_USE_DEFAULT_CXX_LIB AND NOT WIN32)
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
        add_link_options(-stdlib=libstdc++)
    elseif (XRAY_CXX_LIB STREQUAL "libc++")
        add_compile_options(-stdlib=libc++)
        add_link_options(-stdlib=libc++)
        if (CMAKE_SYSTEM_NAME STREQUAL "FreeBSD")
            add_compile_options(-lcxxrt)
            add_link_options(-lcxxrt)
        else()
            add_compile_options(-lc++abi)
            add_link_options(-lc++abi)
        endif()
    endif()
endif()

add_compile_options(-Wno-attributes)
if (APPLE)
    add_compile_options(-Wl,-undefined,error)
elseif (NOT WIN32)
    add_compile_options(-Wl,--no-undefined)
endif()

# TODO test
if (XRAY_USE_ASAN)
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
        -fsanitize=address
        -fsanitize=leak
        -fsanitize=undefined
    )
endif()

if (CMAKE_SYSTEM_PROCESSOR STREQUAL "aarch64" OR CMAKE_SYSTEM_PROCESSOR STREQUAL "arm64")
    set(PROJECT_PLATFORM_ARM64 TRUE)
elseif (CMAKE_SYSTEM_PROCESSOR MATCHES "armv*")
    set(PROJECT_PLATFORM_ARM TRUE)
elseif (CMAKE_SYSTEM_PROCESSOR STREQUAL "e2k")
    set(PROJECT_PLATFORM_E2K TRUE)
elseif (CMAKE_SYSTEM_PROCESSOR STREQUAL "ppc" OR CMAKE_SYSTEM_PROCESSOR STREQUAL "ppc64le")
    set(PROJECT_PLATFORM_PPC TRUE)
endif()

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
    add_compile_options(-Og)
endif()

add_compile_options(
    $<$<AND:$<CONFIG:Release,ReleaseMasterGold>,$<CXX_COMPILER_ID:Clang,AppleClang>>:-gline-tables-only>
    $<$<CONFIG:Release,ReleaseMasterGold>:-ffunction-sections>
    $<$<CONFIG:Release,ReleaseMasterGold>:-fdata-sections>
)

if (APPLE)
    add_link_options($<$<CONFIG:Release,ReleaseMasterGold>:-Wl,-dead_strip>)
elseif (NOT WIN32)
    add_link_options($<$<CONFIG:Release,ReleaseMasterGold>:-Wl,--gc-sections>)
endif()

if (NOT WIN32)
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

message(STATUS "Using ${MEMORY_ALLOCATOR} memory allocator")

get_property(LIB64 GLOBAL PROPERTY FIND_LIBRARY_USE_LIB64_PATHS)

if ("${LIB64}" STREQUAL "TRUE")
    set(LIBSUFFIX 64)
else()
    set(LIBSUFFIX "")
endif()

set(XRAY_ENABLE_WARNINGS
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
    -Wno-trigraphs
    $<$<CONFIG:ReleaseMasterGold>:-Wno-deprecated-declarations>
    $<$<CONFIG:ReleaseMasterGold>:-Wno-microsoft-include>
    $<$<CONFIG:ReleaseMasterGold>:-Wno-missing-field-initializers>
    $<$<CONFIG:ReleaseMasterGold>:-Wno-missing-braces>
    $<$<CONFIG:ReleaseMasterGold>:-Wno-overloaded-virtual>
    $<$<CONFIG:ReleaseMasterGold>:-Wno-unnecessary-virtual-specifier>
    $<$<CONFIG:ReleaseMasterGold>:-Wno-unused-but-set-variable>
    $<$<CONFIG:ReleaseMasterGold>:-Wno-unused-private-field>
    $<$<CONFIG:ReleaseMasterGold>:-Wno-unused-const-variable>
    $<$<CONFIG:ReleaseMasterGold>:-Wno-sign-compare>
    $<$<CONFIG:ReleaseMasterGold>:-Wno-unused-variable>
    $<$<CONFIG:ReleaseMasterGold>:-Wno-unused-function>
    $<$<CONFIG:ReleaseMasterGold>:-Wno-delayed-template-parsing-in-cxx20>
    $<$<CONFIG:ReleaseMasterGold>:-Wno-c++11-narrowing>
    $<$<CXX_COMPILER_ID:GNU>:$<$<COMPILE_LANGUAGE:CXX>:-Wno-class-memaccess>>
    $<$<CXX_COMPILER_ID:GNU>:$<$<COMPILE_LANGUAGE:CXX>:-Wno-interference-size>>
)

add_compile_definitions(
    $<$<CONFIG:ReleaseMasterGold>:_CRT_SECURE_NO_WARNINGS>
    $<$<CONFIG:ReleaseMasterGold>:_CRT_NONSTDC_NO_WARNINGS>
)

set(XRAY_DISABLE_WARNINGS "-w")
