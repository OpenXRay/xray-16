PROJECT(nvtt)

ADD_SUBDIRECTORY(squish)

SET(NVTT_SRCS
	nvtt.h 
	nvtt.cpp
	Compressor.h
	Compressor.cpp
	nvtt_wrapper.h
	nvtt_wrapper.cpp
	CompressDXT.h
	CompressDXT.cpp
	CompressRGB.h
	CompressRGB.cpp
	QuickCompressDXT.h
	QuickCompressDXT.cpp
	OptimalCompressDXT.h
	OptimalCompressDXT.cpp
	SingleColorLookup.h
	CompressionOptions.h
	CompressionOptions.cpp
	InputOptions.h
	InputOptions.cpp
	OutputOptions.h
	OutputOptions.cpp
	cuda/CudaUtils.h
	cuda/CudaUtils.cpp
	cuda/CudaMath.h
	cuda/Bitmaps.h
	cuda/CudaCompressDXT.h
	cuda/CudaCompressDXT.cpp)

IF(CUDA_FOUND)
	ADD_DEFINITIONS(-DHAVE_CUDA)
	WRAP_CUDA(CUDA_SRCS cuda/CompressKernel.cu)
	SET(NVTT_SRCS ${NVTT_SRCS} ${CUDA_SRCS})
	SET(LIBS ${LIBS} ${CUDA_LIBRARIES})
	INCLUDE_DIRECTORIES(${CUDA_INCLUDE_PATH})
ENDIF(CUDA_FOUND)

INCLUDE_DIRECTORIES(${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/.. ${CMAKE_CURRENT_SOURCE_DIR}/../../include)

ADD_DEFINITIONS(-DNVTT_EXPORTS)

IF(NVTT_SHARED)
	ADD_DEFINITIONS(-DNVTT_SHARED=1)
	ADD_LIBRARY(nvtt SHARED ${NVTT_SRCS})
ELSE(NVTT_SHARED)
	ADD_LIBRARY(nvtt ${NVTT_SRCS})
ENDIF(NVTT_SHARED)

TARGET_LINK_LIBRARIES(nvtt ${LIBS} nvcore nvmath nvimage squish)

INSTALL(TARGETS nvtt 
	RUNTIME DESTINATION bin
	LIBRARY DESTINATION lib
	ARCHIVE DESTINATION lib/static)

INSTALL(FILES nvtt.h DESTINATION include/nvtt)



# test executables
ADD_EXECUTABLE(nvcompress tools/compress.cpp tools/cmdline.h)
TARGET_LINK_LIBRARIES(nvcompress nvcore nvmath nvimage nvtt)

ADD_EXECUTABLE(nvdecompress tools/decompress.cpp tools/cmdline.h)
TARGET_LINK_LIBRARIES(nvdecompress nvcore nvmath nvimage)

ADD_EXECUTABLE(nvddsinfo tools/ddsinfo.cpp tools/cmdline.h)
TARGET_LINK_LIBRARIES(nvddsinfo nvcore nvmath nvimage)

ADD_EXECUTABLE(nvimgdiff tools/imgdiff.cpp tools/cmdline.h)
TARGET_LINK_LIBRARIES(nvimgdiff nvcore nvmath nvimage)

ADD_EXECUTABLE(nvassemble tools/assemble.cpp tools/cmdline.h)
TARGET_LINK_LIBRARIES(nvassemble nvcore nvmath nvimage)

ADD_EXECUTABLE(filtertest tests/filtertest.cpp tools/cmdline.h)
TARGET_LINK_LIBRARIES(filtertest nvcore nvmath nvimage)

ADD_EXECUTABLE(nvzoom tools/resize.cpp tools/cmdline.h)
TARGET_LINK_LIBRARIES(nvzoom nvcore nvmath nvimage)

INSTALL(TARGETS nvcompress nvdecompress nvddsinfo nvimgdiff nvassemble nvzoom DESTINATION bin)

# UI tools
IF(QT4_FOUND AND NOT MSVC)
	SET(QT_USE_QTOPENGL TRUE)
	INCLUDE_DIRECTORIES(${QT_INCLUDE_DIR} ${CMAKE_CURRENT_BINARY_DIR})
	
	SET(SRCS
		tools/main.cpp
		tools/configdialog.h
		tools/configdialog.cpp)

	SET(LIBS
		nvtt
		${QT_QTCORE_LIBRARY}
		${QT_QTGUI_LIBRARY}
		${QT_QTOPENGL_LIBRARY})

	QT4_WRAP_UI(UICS tools/configdialog.ui)
	QT4_WRAP_CPP(MOCS tools/configdialog.h)
	#QT4_ADD_RESOURCES(RCCS tools/configdialog.rc)

	ADD_EXECUTABLE(nvcompressui MACOSX_BUNDLE ${SRCS} ${UICS} ${MOCS})
	TARGET_LINK_LIBRARIES(nvcompressui ${LIBS})

ENDIF(QT4_FOUND AND NOT MSVC)


