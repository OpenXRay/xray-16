PROJECT(squish)

INCLUDE_DIRECTORIES(${CMAKE_CURRENT_SOURCE_DIR})

SET(SQUISH_SRCS
	fastclusterfit.cpp
	fastclusterfit.h
	weightedclusterfit.cpp
	weightedclusterfit.h
	colourblock.cpp
	colourblock.h
	colourfit.cpp
	colourfit.h
	colourset.cpp
	colourset.h
	config.h
	maths.cpp
	maths.h
	simd.h
	simd_sse.h
	simd_ve.h)

ADD_LIBRARY(squish STATIC ${SQUISH_SRCS})

IF(CMAKE_COMPILER_IS_GNUCXX)
	SET_TARGET_PROPERTIES(squish PROPERTIES COMPILE_FLAGS -fPIC)
ENDIF(CMAKE_COMPILER_IS_GNUCXX)

