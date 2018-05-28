cmake_minimum_required(VERSION 2.8.0)
project(OpenXRay)

include(${PROJECT_SOURCE_DIR}/cmake/utils.cmake)
set(CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fpermissive --std=c++17")

add_definitions(-D_MT -D_CPPUNWIND -DPURE_DYNAMIC_CAST -DDECLARE_SPECIALIZATION -DM_NOSTDCONTAINERS_EXT -DUSE_OGL)

set(LUA_INCLUDE_DIR Externals/LuaJIT/src)
set(LUA_LIBRARIES luajit)
set(LUA_FOUND 1)
if (NOT WIN32)
find_package(OpenSSL REQUIRED)
find_package(Theora REQUIRED)
find_package(OGG REQUIRED)
find_package(SDL2 REQUIRED)
find_package(LZO REQUIRED)
find_package(JPEG REQUIRED)
endif()

include_directories(${LUA_INCLUDE_DIR})
include_directories(${CMAKE_SOURCE_DIR} {CMAKE_SOURCE_DIR}/Common ${CMAKE_SOURCE_DIR}/Externals ${CMAKE_SOURCE_DIR}/Externals/gli/external/glm ${CMAKE_SOURCE_DIR}/../sdk/include/loki)

add_subdirectory(Externals)
add_subdirectory(src)
