PROJECT(nvcore)
ADD_SUBDIRECTORY(poshlib)

SET(CORE_SRCS
	nvcore.h
	Ptr.h
	BitArray.h
	Memory.h
	Memory.cpp
	Debug.h
	Debug.cpp
	Containers.h
	StrLib.h
	StrLib.cpp
	Stream.h
	StdStream.h
	TextReader.h
	TextReader.cpp
	TextWriter.h
	TextWriter.cpp
	Radix.h
	Radix.cpp
	Library.h
	Library.cpp)

INCLUDE_DIRECTORIES(${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/.. ${CMAKE_CURRENT_SOURCE_DIR}/../../include)

# targets
ADD_DEFINITIONS(-DNVCORE_EXPORTS)

IF(UNIX)
	SET(LIBS ${LIBS} ${CMAKE_DL_LIBS})
ENDIF(UNIX)

IF(NVCORE_SHARED)
	ADD_DEFINITIONS(-DNVCORE_SHARED=1)
	ADD_LIBRARY(nvcore SHARED ${CORE_SRCS})
ELSE(NVCORE_SHARED)
	ADD_LIBRARY(nvcore ${CORE_SRCS})
ENDIF(NVCORE_SHARED)

TARGET_LINK_LIBRARIES(nvcore ${LIBS})

INSTALL(TARGETS nvcore
	RUNTIME DESTINATION bin
	LIBRARY DESTINATION lib
	ARCHIVE DESTINATION lib/static)
