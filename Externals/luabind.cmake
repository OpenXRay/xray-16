cmake_minimum_required(VERSION 2.8)
project(luabind)

option(LUABIND_BUILD_TESTING "Build luabind testing" OFF)
option(LUABIND_BUILD_SHARED "Build luabind as a shared library?" ON)

set(CMAKE_MODULE_PATH "${CMAKE_MODULE_PATH}" "${CMAKE_SOURCE_DIR}/cmake")

if(BUILD_DEPENDS)
   add_subdirectory(luajit ${CMAKE_BINARY_DIR}/Externals/luajit)
else()
   link_directories(${CMAKE_BINARY_DIR}/Externals/luajit)
endif()
#if(BUILD_DEPENDS)
#	add_subdirectory(platform/cmake/LuaJIT ${CMAKE_BINARY_DIR}/LuaJIT)
#else()
#	link_directories(dependencies/LuaJIT/build/)
#endif()

if(MSVC)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /MP")
endif()

if(CMAKE_COMPILER_IS_GNUCXX OR "${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -fpermissive")
endif()

if(LUABIND_BUILD_SHARED)
	add_definitions(-DLUABIND_DYNAMIC_LINK)
endif()

add_definitions(-DNDEBUG) #TODO: Add toggle for debug mode

include_directories(${CMAKE_CURRENT_SOURCE_DIR}
	${LUA_INCLUDE_DIR}
	luabind
)

add_subdirectory(luabind/src)

if(CMAKE_CURRENT_SOURCE_DIR STREQUAL CMAKE_SOURCE_DIR)
	if(LUABIND_BUILD_TESTING)
		add_subdirectory(luabind/test)
	endif()
	add_subdirectory(luabind/doc)
endif()
