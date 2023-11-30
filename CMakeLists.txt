cmake_minimum_required(VERSION 3.16)

project(OpenXRay)

set(CMAKE_CXX_STANDARD 17)

if (CMAKE_VERBOSE_MAKEFILE)
    set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
    set(CMAKE_EXECUTE_PROCESS_COMMAND_ECHO STDOUT)
endif()

if (NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Release" CACHE STRING "" FORCE)
endif()

# Redirecting the default installation path /usr/local to /usr no need to use -DCMAKE_INSTALL_PREFIX =/usr
if (CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
    set(CMAKE_INSTALL_PREFIX "/usr")
endif()

# Installation paths
include(GNUInstallDirs)

set(CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake)
include(${PROJECT_SOURCE_DIR}/cmake/utils.cmake)

# Output all libraries and executable to one folder
set(COMPILE_OUTPUT_FOLDER ${CMAKE_SOURCE_DIR}/bin/${ARCH_TYPE}/${CMAKE_BUILD_TYPE})
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${COMPILE_OUTPUT_FOLDER})
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${COMPILE_OUTPUT_FOLDER})
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${COMPILE_OUTPUT_FOLDER})
set(CMAKE_PDB_OUTPUT_DIRECTORY ${COMPILE_OUTPUT_FOLDER})
set(CMAKE_COMPILE_PDB_OUTPUT_DIRECTORY ${COMPILE_OUTPUT_FOLDER})

if (DISABLE_PORTABLE_MODE)
    add_definitions(-DDISABLE_PORTABLE_MODE)
endif()

if (NOT DEFINED ALLOW_RPATH)
    set(ALLOW_RPATH TRUE)
endif()

if (ALLOW_RPATH)
    # Set the shared object search path for the installation
    set(CMAKE_INSTALL_RPATH ${CMAKE_INSTALL_FULL_LIBDIR})

    # Set the shared object search path for the build tree
    # On Linux/BSD we can use $ORIGIN to search for modules next to the executable
    if (NOT APPLE)
        set(CMAKE_BUILD_RPATH $ORIGIN)
    else()
        set(CMAKE_BUILD_RPATH ${COMPILE_OUTPUT_FOLDER})
    endif()
endif()

message(STATUS "CMAKE_SYSTEM_PROCESSOR: " ${CMAKE_SYSTEM_PROCESSOR})
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

option(MASTER_GOLD "Build with MASTER_GOLD" ${MASTER_GOLD_DEFAULT_VALUE})
message(STATUS "MASTER_GOLD: ${MASTER_GOLD}")
if (MASTER_GOLD)
    add_definitions(-DMASTER_GOLD)
endif()

option(STATIC_BUILD "Use static build" ${MASTER_GOLD})
message(STATUS "STATIC_BUILD: ${STATIC_BUILD}")
if (STATIC_BUILD)
    # XXX: Uncomment only after build with XRAY_STATIC_BUILD is fixed
    add_definitions(-DXRAY_STATIC_BUILD)
    set(LIBS_LINK_TYPE STATIC)
    set(LUABIND_BUILD_SHARED OFF) # xrLuaBind
    set(BUILD_STATIC_LIB ON) # LuaJIT
    set(BUILD_DYNAMIC_LIB OFF)  # LuaJIT
else()
    set(LIBS_LINK_TYPE SHARED)
endif()

function(xr_install tgt)
    if (NOT MSVC)
        install(TARGETS ${tgt} DESTINATION "."
            PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE # chmod 755
        )
    else()
        install(TARGETS ${tgt}
            CONFIGURATIONS Debug
            RUNTIME DESTINATION Debug/
            LIBRARY DESTINATION Debug/
        )
        install(FILES $<TARGET_PDB_FILE:${tgt}>
            CONFIGURATIONS Debug
            DESTINATION Debug/
        )
        install(TARGETS ${tgt}
            CONFIGURATIONS Release
            RUNTIME DESTINATION Release/
            LIBRARY DESTINATION Release/
        )
    endif()
endfunction()

# Use only if install defined outside target directory(like luabind, for example)
function(xr_install_file tgt)
    if (NOT MSVC)
        install(FILES $<TARGET_FILE:${tgt}> DESTINATION "."
            PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE # chmod 755
        )
    else()
        install($<TARGET_FILE:${tgt}>
            CONFIGURATIONS Debug
            RUNTIME DESTINATION Debug/
        )
        install(FILES $<TARGET_PDB_FILE:${tgt}>
            CONFIGURATIONS Debug
            DESTINATION Debug/
        )
        install($<TARGET_FILE:${tgt}>
           CONFIGURATIONS Release
           RUNTIME DESTINATION Release/
        )
    endif()
endfunction()

find_program(CCACHE_FOUND ccache)
if (CCACHE_FOUND)
    set_property(GLOBAL PROPERTY RULE_LAUNCH_COMPILE ccache)
    set_property(GLOBAL PROPERTY RULE_LAUNCH_LINK ccache)
    set(ENV{CCACHE_SLOPPINESS} pch_defines,time_macros)
endif (CCACHE_FOUND)

if (CMAKE_CXX_COMPILER_ID MATCHES "GNU")
    if (CMAKE_CXX_COMPILER_VERSION VERSION_LESS 8.0 AND NOT PROJECT_PLATFORM_E2K)
        message(FATAL_ERROR "Building with a gcc version less than 8.0 is not supported.")
    elseif (CMAKE_CXX_COMPILER_VERSION VERSION_LESS 7.0 AND PROJECT_PLATFORM_E2K)
        message(FATAL_ERROR "Building with a MCST lcc version less than 1.25 is not supported.")
    endif()
elseif (CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    # XXX: Remove -fdelayed-template-parsing
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fdelayed-template-parsing -Wno-unused-command-line-argument -Wno-inconsistent-missing-override")
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
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -stdlib=libstdc++")
    elseif (XRAY_CXX_LIB STREQUAL "libc++")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -stdlib=libc++")
        if (CMAKE_SYSTEM_NAME STREQUAL "FreeBSD")
            set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -stdlib=libc++ -lcxxrt")
        else()
            set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -stdlib=libc++ -lc++abi")
        endif()
    endif()
endif()

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-attributes")
if (APPLE)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wl,-undefined,error")
else()
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wl,--no-undefined")
endif()

option(USE_ADDRESS_SANITIZER "Use AddressSanitizer" OFF)
if (USE_ADDRESS_SANITIZER)
    set(SANITIZE_FLAGS
        "-fsanitize=address"
        "-fsanitize=leak"
        "-fno-omit-frame-pointer"
        "-fno-optimize-sibling-calls"
        "-fsanitize=undefined"
        "-fno-sanitize=vptr"
    )

    if (CMAKE_BUILD_TYPE STREQUAL "Debug")
        list(APPEND SANITIZE_FLAGS "-g")
    endif()

    if (CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        add_link_options("-shared-libasan")
    endif()
endif()
string(REPLACE ";" " " SANITIZE_FLAGS "${SANITIZE_FLAGS}")

message(STATUS "USE_ADDRESS_SANITIZER: ${USE_ADDRESS_SANITIZER}")

option(USE_LTO "Use Link Time Optimization" ${MASTER_GOLD})
if (USE_LTO)
    include(CheckIPOSupported)
    check_ipo_supported(RESULT LTO_SUPPORTED)

    if (LTO_SUPPORTED)
        # With clang cmake only enables '-flto=thin' but we want full LTO
        if (CMAKE_CXX_COMPILER_ID MATCHES "Clang")
            set(LTO_FLAGS "-flto=full")
        else()
            set(CMAKE_INTERPROCEDURAL_OPTIMIZATION ON)
        endif()
    endif()
endif()

message(STATUS "USE_LTO: ${USE_LTO}")

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${SANITIZE_FLAGS} ${LTO_FLAGS}")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${SANITIZE_FLAGS}")
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${SANITIZE_FLAGS} ${LTO_FLAGS}")
set(CMAKE_MODULE_LINKER_FLAGS "${CMAKE_MODULE_LINKER_FLAGS} ${SANITIZE_FLAGS} ${LTO_FLAGS}")

set(LUA_LIBRARIES xrLuajit)

if (PROJECT_PLATFORM_ARM)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -mfpu=neon")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -mfpu=neon")
elseif (PROJECT_PLATFORM_ARM64)
    set(CMAKE_C_FLAGS ${CMAKE_C_FLAGS})
elseif (PROJECT_PLATFORM_E2K)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wno-unknown-pragmas")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-unknown-pragmas")
elseif (PROJECT_PLATFORM_PPC)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DNO_WARN_X86_INTRINSICS")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -maltivec -mabi=altivec -DNO_WARN_X86_INTRINSICS")
else()
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -mfpmath=sse -msse3")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -mfpmath=sse -msse3")
endif()

option(XRAY_USE_DEFAULT_LINKER "Don't select linker, use system default." OFF)

if (NOT XRAY_LINKER AND NOT XRAY_USE_DEFAULT_LINKER AND NOT APPLE) # XXX: check if we can remove check for Apple
    include(CheckCXXCompilerFlag)
    CHECK_CXX_COMPILER_FLAG("-fuse-ld=gold" GOLD_LINKER_AVAILABLE)
    CHECK_CXX_COMPILER_FLAG("-fuse-ld=lld" LLD_LINKER_AVAILABLE)

    if (CMAKE_CXX_COMPILER_ID MATCHES "Clang" AND LLD_LINKER_AVAILABLE)
        set(XRAY_LINKER "lld" CACHE STRING "" FORCE)
    elseif (GOLD_LINKER_AVAILABLE)
        set(XRAY_LINKER "gold" CACHE STRING "" FORCE)
    endif()
endif()

if (XRAY_LINKER AND NOT XRAY_USE_DEFAULT_LINKER)
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -fuse-ld=${XRAY_LINKER}")
    set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -fuse-ld=${XRAY_LINKER}")
endif()

if (CMAKE_BUILD_TYPE STREQUAL "Debug")
    add_definitions(-DDEBUG -DMIXED)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Og")
    set(LUA_LIBRARIES xrLuajit-debug)
endif()

set(LUA_FOUND true)

message("CMake build ${CMAKE_BUILD_TYPE}, CFLAGS=${CMAKE_C_FLAGS}, CXXFLAGS=${CMAKE_CXX_FLAGS}")

add_definitions(-D_MT -D_CPPUNWIND -DPURE_DYNAMIC_CAST -DDECLARE_SPECIALIZATION -DM_NOSTDCONTAINERS_EXT)
add_definitions(-DLUABIND_DYNAMIC_LINK) #self-explanatory
add_definitions(-DdSINGLE) # for ODE and xrPhysics
if (MASTER_GOLD)
    add_definitions(-DLUABIND_NO_EXCEPTIONS)
    add_definitions(-DLUABIND_NO_ERROR_CHECKING)
    add_definitions(-DdNODEBUG)
endif()

set(LUA_INCLUDE_DIR Externals/LuaJIT/src)

if (NOT WIN32)
    find_package(SDL2 REQUIRED)
    find_package(OpenGL REQUIRED)
    find_package(GLEW REQUIRED)
    find_package(OpenAL REQUIRED)
    find_package(JPEG)
    if (JPEG_FOUND)
        set(USE_CXIMAGE TRUE)
    endif()
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

if (MEMORY_ALLOCATOR STREQUAL "mimalloc")
    add_definitions(-DUSE_MIMALLOC)
    if (NOT mimalloc_FOUND)
        message(FATAL_ERROR "mimalloc allocator requested but not found. Please, install mimalloc package or select standard allocator.")
    endif()
else()
    add_definitions(-DUSE_PURE_ALLOC)
endif()

message("Using ${MEMORY_ALLOCATOR} memory allocator")

# XXX: move to LuaJIT
include_directories(${LUA_INCLUDE_DIR})

execute_process(COMMAND git rev-parse --verify HEAD
    WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
    OUTPUT_VARIABLE GIT_SHA1
    ERROR_QUIET
    OUTPUT_STRIP_TRAILING_WHITESPACE
)

message("git commit: " ${GIT_SHA1})

execute_process(COMMAND git rev-parse --abbrev-ref HEAD
    WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
    OUTPUT_VARIABLE GIT_BRANCH
    ERROR_QUIET
    OUTPUT_STRIP_TRAILING_WHITESPACE
)

message("git branch: " ${GIT_BRANCH})

include_directories(
    ${CMAKE_SOURCE_DIR}/Externals
    ${CMAKE_SOURCE_DIR}/Externals/luabind
    ${CMAKE_SOURCE_DIR}/Externals/imgui
)

add_subdirectory(Externals)

set(CMAKE_CXX_FLAGS
    "${CMAKE_CXX_FLAGS}"
    "-Wall"
    #"-Werror"
    "-Wextra"
    #"-pedantic"
    "-Wno-unknown-pragmas"
    "-Wno-strict-aliasing"
    "-Wno-parentheses"
    "-Wno-unused-label"
    "-Wno-unused-parameter"
    "-Wno-switch"
    #"-Wno-padded"
    #"-Wno-c++98-compat"
    #"-Wno-c++98-compat-pedantic"
    #"-Wno-c++11-compat"
    #"-Wno-c++11-compat-pedantic"
    #"-Wno-c++14-compat"
    #"-Wno-c++14-compat-pedantic"
    #"-Wno-newline-eof"
)
if (CMAKE_CXX_COMPILER_ID MATCHES "GNU")
    set(CMAKE_CXX_FLAGS
        "${CMAKE_CXX_FLAGS}"
        "-Wno-class-memaccess"
        "-Wno-interference-size"
    )
endif()
string(REPLACE ";" " " CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")

add_subdirectory(src)
add_subdirectory(res)
add_subdirectory(misc)

get_property(LIB64 GLOBAL PROPERTY FIND_LIBRARY_USE_LIB64_PATHS)

if ("${LIB64}" STREQUAL "TRUE")
    set(LIBSUFFIX 64)
else()
    set(LIBSUFFIX "")
endif()
