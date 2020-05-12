cmake_minimum_required(VERSION 2.8.0)
# Redirecting the default installation path /usr/local to /usr no need to use -DCMAKE_INSTALL_PREFIX =/usr
set(CMAKE_INSTALL_PREFIX /usr)
project(OpenXRay)

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
if(${CMAKE_SYSTEM_PROCESSOR} MATCHES "armv*")
	set(PROJECT_PLATFORM_ARM TRUE)
elseif(${CMAKE_SYSTEM_PROCESSOR} STREQUAL "aarch64")
	set(PROJECT_PLATFORM_ARM64 TRUE)
elseif(${CMAKE_SYSTEM_PROCESSOR} STREQUAL "e2k")
	set(PROJECT_PLATFORM_E2K TRUE)
endif()

if(NOT WIN32)
    IF(EXISTS "${CMAKE_ROOT}/Modules/CPack.cmake")
        set(CPACK_PACKAGE_NAME "openxray")
        set(CPACK_FILE_NAME "openxray")
        set(CPACK_DEBIAN_FILE_NAME "DEB-DEFAULT")
        set(CPACK_PACKAGE_VERSION "1.6.02")
        set(CPACK_PACKAGE_CONTACT "OpenXRay <openxray@yahoo.com>")
        set(CPACK_PACKAGE_DESCRIPTION "OpenXRay is an improved version of the X-Ray Engine – game engine used in world famous S.T.A.L.K.E.R. game series by GSC Game World.")
        set(CPACK_PACKAGE_DESCRIPTION_SUMMARY ${CPACK_PACKAGE_DESCRIPTION})
        set(CPACK_DEBIAN_PACKAGE_SECTION "games")
        set(CPACK_DEBIAN_PACKAGE_HOMEPAGE "https://github.com/OpenXRay/xray-16")
        set(CPACK_DEBIAN_PACKAGE_SHLIBDEPS ON)
        set(CPACK_STRIP_FILES TRUE)
        set(CPACK_GENERATOR DEB)
        set(CPACK_SOURCE_IGNORE_FILES "/.gitattributes")
        set(CPACK_RESOURCE_FILE_README ${PROJECT_SOURCE_DIR}/README.md)
        set(CPACK_RESOURCE_FILE_LICENSE ${PROJECT_SOURCE_DIR}/License.txt)
        set(CPACK_DEBIAN_PACKAGE_CONTROL_STRICT_PERMISSION TRUE)
        set(CPACK_DEBIAN_REVISON "ubuntu-bionic-a1")
        set(CPACK_PACKAGE_FILE_NAME "${CPACK_PACKAGE_NAME}-${CPACK_PACKAGE_VERSION}-${CPACK_DEBIAN_REVISON}_${_OS_ARCH}")
        set(CPACK_DEBIAN_PACKAGE_SOURCE "OpenXRay")
        INCLUDE(CPack)
    ENDIF(EXISTS "${CMAKE_ROOT}/Modules/CPack.cmake")
endif()

option ( TBB_PARALLEL "Use tbb::parallel for prarticle and skinning acceleration on SMP." ON )
option ( USE_CRYPTOPP "Use Crypto++ Library (REQUIRED FOR MULTIPLAYER)" ON )

# Memory allocator option
set ( MEMORY_ALLOCATOR "mimalloc" CACHE STRING "Use specific memory allocator (mimalloc/tbb/standard)" )
message( "Using ${MEMORY_ALLOCATOR} memory allocator" )
if ( MEMORY_ALLOCATOR STREQUAL "mimalloc" )
    add_definitions ( -DUSE_MIMALLOC )
elseif ( MEMORY_ALLOCATOR STREQUAL "tbb" )
    add_definitions ( -DUSE_TBB_MALLOC )
else()
    add_definitions ( -DUSE_PURE_ALLOC )
endif()

include(cotire)

function(xr_install tgt)
	if(NOT MSVC)
		install(TARGETS ${tgt} DESTINATION "."
			PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE
				GROUP_READ GROUP_EXECUTE
				WORLD_READ WORLD_EXECUTE) # chmod 755
	else()
		install(TARGETS ${tgt}
			CONFIGURATIONS Debug
			RUNTIME DESTINATION Debug/
			LIBRARY DESTINATION Debug/)
		install(FILES $<TARGET_PDB_FILE:${tgt}>
			CONFIGURATIONS Debug
			DESTINATION Debug/ )
		install(TARGETS ${tgt}
			CONFIGURATIONS Release
			RUNTIME DESTINATION Release/
			LIBRARY DESTINATION Release/)
	endif()
endfunction()

# Use only if install defined outside target directory(like luabind, for example)
function(xr_install_file tgt)
        if(NOT MSVC)
                install(FILES $<TARGET_FILE:${tgt}> DESTINATION "."
                        PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE
                                GROUP_READ GROUP_EXECUTE
                                WORLD_READ WORLD_EXECUTE) # chmod 755
        else()
                install($<TARGET_FILE:${tgt}>
                        CONFIGURATIONS Debug
                        RUNTIME DESTINATION Debug/)
                install(FILES $<TARGET_PDB_FILE:${tgt}>
                        CONFIGURATIONS Debug
                        DESTINATION Debug/ )
                install($<TARGET_FILE:${tgt}>
                        CONFIGURATIONS Release
                        RUNTIME DESTINATION Release/)
        endif()
endfunction()

find_program(CCACHE_FOUND ccache)
if(CCACHE_FOUND)
    set_property(GLOBAL PROPERTY RULE_LAUNCH_COMPILE ccache)
    set_property(GLOBAL PROPERTY RULE_LAUNCH_LINK ccache)
    set(ENV{CCACHE_SLOPPINESS} pch_defines,time_macros)
endif(CCACHE_FOUND)

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU")
    if(CMAKE_CXX_COMPILER_VERSION VERSION_LESS 8.0)
        message(FATAL_ERROR "Building with a gcc version less than 8.0 is not supported.")
    endif()
elseif(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-unused-command-line-argument -Wno-inconsistent-missing-override")
    if(NOT USED_CXX_LIB)
        set(USED_CXX_LIB "libstdcpp" CACHE STRING "" FORCE)
    endif()
    if(${USED_CXX_LIB} STREQUAL "libstdcpp")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -stdlib=libstdc++")
    elseif(${USED_CXX_LIB} STREQUAL "libcpp")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -stdlib=libc++")
        set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -stdlib=libc++ -lc++abi")
    endif()
endif()

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} --std=c++17 -Wno-attributes -Wl,--no-undefined -fvisibility=hidden")

#set (SANITIZE_FLAGS "-fsanitize=address  -fsanitize=leak -fno-omit-frame-pointer -g -fsanitize=undefined -fno-sanitize=vptr")
#if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
#    set (SANITIZE_FLAGS "-shared-libasan ${SANITIZE_FLAGS}")
#endif()
set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${SANITIZE_FLAGS}")
set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${SANITIZE_FLAGS}")
set (CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${SANITIZE_FLAGS}")
set (CMAKE_MODULE_LINKER_FLAGS "${CMAKE_MODULE_LINKER_FLAGS} ${SANITIZE_FLAGS}")

set(LUA_LIBRARIES xrLuajit)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Release" CACHE STRING "" FORCE)
endif()

if(PROJECT_PLATFORM_ARM)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -mfpu=neon")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -mfpu=neon")
elseif(PROJECT_PLATFORM_ARM64)
    set(CMAKE_C_FLAGS ${CMAKE_C_FLAGS})
elseif(PROJECT_PLATFORM_E2K)
    set(CMAKE_C_FLAGS ${CMAKE_C_FLAGS})
    set(CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS})
else()
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -mfpmath=sse -msse3")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -mfpmath=sse -msse3")
endif()

set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -fuse-ld=gold")
set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -fuse-ld=gold")

if( TBB_PARALLEL )
  add_definitions ( -DUSE_TBB_PARALLEL )
endif()

if( USE_CRYPTOPP )
  add_definitions ( -DUSE_CRYPTOPP )
endif()

if(${CMAKE_BUILD_TYPE} STREQUAL "Debug")
    add_definitions(-DDEBUG -DMIXED)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Og")
    set(LUA_LIBRARIES xrLuajit-debug)
endif()

if (CMAKE_BUILD_TYPE STREQUAL Release)
    set(CMAKE_SKIP_BUILD_RPATH TRUE)
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
            COMMAND ${CMAKE_STRIP} ${PROJECT_NAME})
endif ()

set(LUA_FOUND true)

message("CMake build ${CMAKE_BUILD_TYPE}, CFLAGS=${CMAKE_C_FLAGS}, CXXFLAGS=${CMAKE_CXX_FLAGS}")

add_definitions(-D_MT -D_CPPUNWIND -DPURE_DYNAMIC_CAST -DDECLARE_SPECIALIZATION -DM_NOSTDCONTAINERS_EXT)
add_definitions(-DLUABIND_DYNAMIC_LINK) #self-explanatory
add_definitions(-DdSINGLE) # for ODE

set(LUA_INCLUDE_DIR Externals/LuaJIT/src)

if (NOT WIN32)
    find_package(GLEW REQUIRED)
    find_package(FreeImage REQUIRED)
    find_package(LockFile REQUIRED)
    find_package(OpenAL REQUIRED)
    if (USE_CRYPTOPP)
        find_package(Crypto++ REQUIRED)
    endif()
    find_package(Theora REQUIRED)
    find_package(OGG REQUIRED)
    find_package(SDL2 REQUIRED)
    find_package(LZO REQUIRED)
    find_package(JPEG REQUIRED)
    if (TBB_PARALLEL)
        find_package(TBB REQUIRED)
    endif()
    find_package(PCRE REQUIRED)
endif()

include_directories(${LUA_INCLUDE_DIR})
include_directories(${CMAKE_SOURCE_DIR} ${CMAKE_SOURCE_DIR}/Common )

add_subdirectory(Externals)
add_subdirectory(src)
add_subdirectory(res)

# Unix system configuration
if (UNIX)
    # --------------------------------------------------
    # Uninstall target
    # --------------------------------------------------
    # To clean system folder from libraries and binaries
    # that was installed with `sudo make install`
    # just run `sudo make uninstall`
    #
    if(NOT TARGET uninstall)
        configure_file(
                "${CMAKE_CURRENT_SOURCE_DIR}/cmake/cmake_uninstall.cmake.in"
                "${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake"
                IMMEDIATE @ONLY)

        add_custom_target(uninstall COMMAND ${CMAKE_COMMAND} -P ${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake)
    endif()
endif()

get_property(LIB64 GLOBAL PROPERTY FIND_LIBRARY_USE_LIB64_PATHS)

if ("${LIB64}" STREQUAL "TRUE")
    set(LIBSUFFIX 64)
else()
    set(LIBSUFFIX "")
endif()
