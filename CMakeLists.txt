cmake_minimum_required(VERSION 2.8.0)
project(OpenXRay)

include(${PROJECT_SOURCE_DIR}/cmake/utils.cmake)

set(CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake)

find_program(CCACHE_FOUND ccache)
if(CCACHE_FOUND)
    set_property(GLOBAL PROPERTY RULE_LAUNCH_COMPILE ccache)
    set_property(GLOBAL PROPERTY RULE_LAUNCH_LINK ccache)
endif(CCACHE_FOUND)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fpermissive --std=c++17")
#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fpermissive") This is for the future versions
#set(CMAKE_CXX_STANDARD 14) #Otherwise adds -std=gnu++11 and breaks successful building
#set(CMAKE_CXX_STANDARD_REQUIRED ON)
#set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Release" CACHE STRING "" FORCE)
    set(LUA_LIBRARIES luajit)
elseif(${CMAKE_BUILD_TYPE} STREQUAL "RELWITHDEBINFO")
    set(LUA_LIBRARIES luajit)
else()
    set(LUA_LIBRARIES luajit-debug)
endif()

message("CMake build ${CMAKE_BUILD_TYPE}")

add_definitions(-D_MT -D_CPPUNWIND -DPURE_DYNAMIC_CAST -DDECLARE_SPECIALIZATION -DM_NOSTDCONTAINERS_EXT -DUSE_OGL)

set(LUA_INCLUDE_DIR Externals/LuaJIT/src)

set(LUA_FOUND 1)
if (NOT WIN32)
find_package(Crypto++ REQUIRED)
find_package(PugiXML REQUIRED)
find_package(Theora REQUIRED)
find_package(OGG REQUIRED)
find_package(SDL2 REQUIRED)
find_package(LZO REQUIRED)
find_package(JPEG REQUIRED)
#this is a temporary solution until find_package will not be fixed
set(TBB_LIBRARIES tbb tbbmalloc_proxy tbbmalloc)
endif()

include_directories(${LUA_INCLUDE_DIR})
include_directories(${CMAKE_SOURCE_DIR} {CMAKE_SOURCE_DIR}/Common ${CMAKE_SOURCE_DIR}/Externals ${CMAKE_SOURCE_DIR}/Externals/gli/external/glm ${CMAKE_SOURCE_DIR}/../sdk/include/loki)

add_subdirectory(Externals)
add_subdirectory(src)
