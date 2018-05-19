cmake_minimum_required(VERSION 2.8.0)
project(cximage)

set(SOURCES
	ximacfg.cpp
	ximaenc.cpp
	ximaexif.cpp
	ximage.cpp
	ximainfo.cpp
	ximaint.cpp
	ximajpg.cpp
	ximalpha.cpp
	ximalyr.cpp
	ximapal.cpp
	ximasel.cpp
	ximath.cpp
	xmemfile.cpp
)

set(HEADERS
	xfile.h
	ximacfg.h
	ximadef.h
	ximage.h
	ximaiter.h
	ximajpg.h
	ximath.h
	xiofile.h
	xmemfile.h
)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fpermissive --std=c++14")
add_definitions(-DCXIMAGE_BUILD)

include_directories("." ${CMAKE_SOURCE_DIR})

add_library(cximage STATIC ${SOURCES} ${HEADERS})

set_target_properties(cximage PROPERTIES PREFIX "")
target_link_libraries(cximage)
