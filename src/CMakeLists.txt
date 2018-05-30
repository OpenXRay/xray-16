cmake_minimum_required(VERSION 3.5)

project(XRAY)

include(${CMAKE_SOURCE_DIR}/utils.cmake)

set(CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake)

include_directories (
	"${PROJECT_SOURCE_DIR}"
	"${PROJECT_SOURCE_DIR}/src"
	)

add_definitions(
	-D_MT
	-D_CPPUNWIND
	-DPURE_DYNAMIC_CAST
	-DDECLARE_SPECIALIZATION
	-DM_NOSTDCONTAINERS_EXT
	-DUSE_OGL
	)

add_compile_options(-std=c++14 -fpermissive)

add_library(tbb SHARED IMPORTED GLOBAL)
add_library(lzo SHARED IMPORTED GLOBAL)
add_library(pugixml SHARED IMPORTED GLOBAL)
add_library(SDL2 SHARED IMPORTED GLOBAL)


add_subdirectory(
	xrCore
	)
#	xrEngine
#	xrGame
#	xrParticles
#	xrPhysics
#	xrScriptEngine
#	xrSound
