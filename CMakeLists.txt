cmake_minimum_required(VERSION 3.16)

project(OpenXRay)

set(CMAKE_CXX_STANDARD 17)

if (CMAKE_VERBOSE_MAKEFILE)
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

message(STATUS "CMAKE_SYSTEM_PROCESSOR: " ${CMAKE_SYSTEM_PROCESSOR})
if (CMAKE_SYSTEM_PROCESSOR MATCHES "armv*")
    set(PROJECT_PLATFORM_ARM TRUE)
elseif (CMAKE_SYSTEM_PROCESSOR STREQUAL "aarch64")
    set(PROJECT_PLATFORM_ARM64 TRUE)
elseif (CMAKE_SYSTEM_PROCESSOR STREQUAL "e2k")
    set(PROJECT_PLATFORM_E2K TRUE)
endif()

if (NOT MASTER_GOLD)
    if (CMAKE_BUILD_TYPE STREQUAL "Release" OR CMAKE_BUILD_TYPE STREQUAL "RelWithDebInfo")
        set(MASTER_GOLD ON CACHE BOOL "" FORCE)
    else()
        set(MASTER_GOLD OFF CACHE BOOL "" FORCE)
    endif()
endif()

message(STATUS "MASTER_GOLD: ${MASTER_GOLD}")
if (MASTER_GOLD)
    add_definitions(-DMASTER_GOLD)
endif()

option(USE_CRYPTOPP "Use Crypto++ Library (REQUIRED FOR MULTIPLAYER)" OFF)

# Memory allocator option
set(MEMORY_ALLOCATOR "mimalloc" CACHE STRING "Use specific memory allocator (mimalloc/standard)")
message("Using ${MEMORY_ALLOCATOR} memory allocator")
if (MEMORY_ALLOCATOR STREQUAL "mimalloc")
    add_definitions(-DUSE_MIMALLOC)
else()
    add_definitions(-DUSE_PURE_ALLOC)
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
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-unused-command-line-argument -Wno-inconsistent-missing-override")
    if (NOT USED_CXX_LIB)
        set(USED_CXX_LIB "libstdcpp" CACHE STRING "" FORCE)
    endif()
    if (USED_CXX_LIB STREQUAL "libstdcpp")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -stdlib=libstdc++")
    elseif (USED_CXX_LIB STREQUAL "libcpp")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -stdlib=libc++")
        set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -stdlib=libc++ -lc++abi")
    endif()
endif()

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-attributes -Wl,--no-undefined -fvisibility=hidden")

#set(SANITIZE_FLAGS "-fsanitize=address  -fsanitize=leak -fno-omit-frame-pointer -g -fsanitize=undefined -fno-sanitize=vptr")
#if (CMAKE_CXX_COMPILER_ID MATCHES "Clang")
#    set(SANITIZE_FLAGS "-shared-libasan ${SANITIZE_FLAGS}")
#endif()

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${SANITIZE_FLAGS}")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${SANITIZE_FLAGS}")
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${SANITIZE_FLAGS}")
set(CMAKE_MODULE_LINKER_FLAGS "${CMAKE_MODULE_LINKER_FLAGS} ${SANITIZE_FLAGS}")

set(LUA_LIBRARIES xrLuajit)

if (PROJECT_PLATFORM_ARM)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -mfpu=neon")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -mfpu=neon")
elseif (PROJECT_PLATFORM_ARM64)
    set(CMAKE_C_FLAGS ${CMAKE_C_FLAGS})
elseif (PROJECT_PLATFORM_E2K)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wno-unknown-pragmas")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-unknown-pragmas")
else()
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -mfpmath=sse -msse3")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -mfpmath=sse -msse3")
endif()

set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -fuse-ld=gold")
set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -fuse-ld=gold")

if (USE_CRYPTOPP)
    add_definitions(-DUSE_CRYPTOPP)
endif()

if (CMAKE_BUILD_TYPE STREQUAL "Debug")
    add_definitions(-DDEBUG -DMIXED)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Og")
    set(LUA_LIBRARIES xrLuajit-debug)
endif()

if (CMAKE_BUILD_TYPE STREQUAL "Release")
    set(CMAKE_SKIP_BUILD_RPATH TRUE)
    #add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD COMMAND ${CMAKE_STRIP} ${PROJECT_NAME})
endif()

set(LUA_FOUND true)

message("CMake build ${CMAKE_BUILD_TYPE}, CFLAGS=${CMAKE_C_FLAGS}, CXXFLAGS=${CMAKE_CXX_FLAGS}")

add_definitions(-D_MT -D_CPPUNWIND -DPURE_DYNAMIC_CAST -DDECLARE_SPECIALIZATION -DM_NOSTDCONTAINERS_EXT)
add_definitions(-DLUABIND_DYNAMIC_LINK) #self-explanatory
add_definitions(-DdSINGLE) # for ODE and xrPhysics

set(LUA_INCLUDE_DIR Externals/LuaJIT/src)

if (NOT WIN32)
    find_package(GLEW REQUIRED)
    find_package(FreeImage REQUIRED)
    find_package(FreeImagePlus REQUIRED)
    find_package(LockFile REQUIRED)
    find_package(OpenAL REQUIRED)
    if (USE_CRYPTOPP)
        find_package(Crypto++ REQUIRED)
    endif()
    find_package(Theora REQUIRED)
    find_package(Ogg REQUIRED)
    find_package(SDL REQUIRED)
    find_package(LZO REQUIRED)
    find_package(JPEG REQUIRED)
    find_package(PCRE REQUIRED)
endif()

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
)

add_subdirectory(Externals)
add_subdirectory(src)
add_subdirectory(res)

get_property(LIB64 GLOBAL PROPERTY FIND_LIBRARY_USE_LIB64_PATHS)

if ("${LIB64}" STREQUAL "TRUE")
    set(LIBSUFFIX 64)
else()
    set(LIBSUFFIX "")
endif()
