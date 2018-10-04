cmake_minimum_required(VERSION 2.8.0)
project(OpenXRay)

include(${PROJECT_SOURCE_DIR}/cmake/utils.cmake)

set(CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake)
set(CMAKE_SKIP_BUILD_RPATH TRUE)
if(NOT WIN32)
	set(CMAKE_INSTALL_PREFIX "") # Skip all *nix-style installing for a while. Use DESTDIR
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
    if(CMAKE_CXX_COMPILER_VERSION VERSION_LESS 7.0)
        message(FATAL_ERROR "Building with a gcc version less than 7.0 is not supported.")
    endif()
endif()

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fpermissive --std=c++17 -Wno-attributes -pipe")
#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fpermissive") This is for the future versions
#set(CMAKE_CXX_STANDARD 14) #Otherwise adds -std=gnu++11 and breaks successful building
#set(CMAKE_CXX_STANDARD_REQUIRED ON)
#set(CMAKE_CXX_EXTENSIONS OFF)

set(LUA_LIBRARIES luajit)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Release" CACHE STRING "" FORCE)
elseif(${CMAKE_BUILD_TYPE} STREQUAL "Debug")
    add_definitions(-DDEBUG)
    set(LUA_LIBRARIES luajit-debug)
endif()

set(LUA_FOUND true)

message("CMake build ${CMAKE_BUILD_TYPE}, CFLAGS=${CMAKE_C_FLAGS}, CXXFLAGS=${CMAKE_CXX_FLAGS}")

add_definitions(-D_MT -D_CPPUNWIND -DPURE_DYNAMIC_CAST -DDECLARE_SPECIALIZATION -DM_NOSTDCONTAINERS_EXT -DUSE_OGL)

set(LUA_INCLUDE_DIR Externals/LuaJIT/src)

if (NOT WIN32)
if(EXISTS ${CMAKE_SOURCE_DIR}/Externals/cryptopp/dsa.h)
    file(REMOVE_RECURSE ${CMAKE_SOURCE_DIR}/Externals/cryptopp)
    file(MAKE_DIRECTORY ${CMAKE_SOURCE_DIR}/Externals/cryptopp)
endif()
find_package(GLEW REQUIRED)
find_package(FreeImage REQUIRED)
find_package(LockFile REQUIRED)
find_package(OpenAL REQUIRED)
find_package(Crypto++ REQUIRED)
find_package(PugiXML REQUIRED)
find_package(Theora REQUIRED)
find_package(OGG REQUIRED)
find_package(SDL2 REQUIRED)
find_package(LZO REQUIRED)
find_package(JPEG REQUIRED)
find_package(TBB REQUIRED)
endif()

include_directories(${LUA_INCLUDE_DIR})
include_directories(${CMAKE_SOURCE_DIR} {CMAKE_SOURCE_DIR}/Common ${CMAKE_SOURCE_DIR}/Externals ${CMAKE_SOURCE_DIR}/Externals/gli/external/glm ${CMAKE_SOURCE_DIR}/../sdk/include/loki)

add_subdirectory(Externals)
add_subdirectory(src)
