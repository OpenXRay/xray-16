PROJECT(nvtt)

ADD_SUBDIRECTORY(squish)

set(NVTT_SHARED OFF)
set(NVTT_TEST_EXECUTABLES ON)

set(SRC_FILES
    "nvtt.h"
    "nvtt.cpp"
    "Compressor.h"
    "Compressor.cpp"
    "nvtt_wrapper.h"
    "nvtt_wrapper.cpp"
    "CompressDXT.h"
    "CompressDXT.cpp"
    "CompressRGB.h"
    "CompressRGB.cpp"
    "QuickCompressDXT.h"
    "QuickCompressDXT.cpp"
    "OptimalCompressDXT.h"
    "OptimalCompressDXT.cpp"
    "SingleColorLookup.h"
    "CompressionOptions.h"
    "CompressionOptions.cpp"
    "InputOptions.h"
    "InputOptions.cpp"
    "OutputOptions.h"
    "OutputOptions.cpp"
    "cuda/CudaUtils.h"
    "cuda/CudaUtils.cpp"
    "cuda/CudaMath.h"
    "cuda/Bitmaps.h"
    "cuda/CudaCompressDXT.h"
    "cuda/CudaCompressDXT.cpp"
)

# XXX: add "tests" and "tools" folders?

if (CUDA_FOUND)
    WRAP_CUDA(CUDA_SRCS cuda/CompressKernel.cu)
    list(APPEND SRC_FILES ${CUDA_SRCS})
endif()

group_sources(SRC_FILES)

if (NVTT_SHARED)
    add_library(${PROJECT_NAME} SHARED ${SRC_FILES})
else()
    add_library(${PROJECT_NAME} STATIC ${SRC_FILES})
endif()

target_include_directories(${PROJECT_NAME}
    PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/Externals/NVTT/src
    ${CMAKE_SOURCE_DIR}/Externals/NVTT/include
    $<$<BOOL:CUDA_FOUND>:${CUDA_INCLUDE_PATH}>
)

target_link_libraries(${PROJECT_NAME}
    PRIVATE
    nvcore
    nvmath
    nvimage
    squish
    $<$<BOOL:CUDA_FOUND>:${CUDA_LIBRARIES}>
)

target_compile_definitions(${PROJECT_NAME}
    PRIVATE
    -DNVTT_EXPORTS
    $<$<BOOL:NVTT_SHARED>:-DNVTT_SHARED=1>
    $<$<BOOL:CUDA_FOUND>:-DHAVE_CUDA>
)

install(TARGETS ${PROJECT_NAME} LIBRARY
    DESTINATION ${CMAKE_INSTALL_LIBDIR}
    PERMISSIONS OWNER_READ OWNER_WRITE GROUP_READ WORLD_READ
)

install(FILES "nvtt.h" DESTINATION "include/nvtt")

if (NVTT_TEST_EXECUTABLES)
    add_executable(nvcompress tools/compress.cpp tools/cmdline.h)
    target_link_libraries(nvcompress nvcore nvmath nvimage nvtt)

    add_executable(nvdecompress tools/decompress.cpp tools/cmdline.h)
    target_link_libraries(nvdecompress nvcore nvmath nvimage)

    add_executable(nvddsinfo tools/ddsinfo.cpp tools/cmdline.h)
    target_link_libraries(nvddsinfo nvcore nvmath nvimage)

    add_executable(nvimgdiff tools/imgdiff.cpp tools/cmdline.h)
    target_link_libraries(nvimgdiff nvcore nvmath nvimage)

    add_executable(nvassemble tools/assemble.cpp tools/cmdline.h)
    target_link_libraries(nvassemble nvcore nvmath nvimage)

    add_executable(filtertest tests/filtertest.cpp tools/cmdline.h)
    target_link_libraries(filtertest nvcore nvmath nvimage)

    add_executable(nvzoom tools/resize.cpp tools/cmdline.h)
    target_link_libraries(nvzoom nvcore nvmath nvimage)

    install(TARGETS nvcompress nvdecompress nvddsinfo nvimgdiff nvassemble nvzoom
        DESTINATION ${CMAKE_INSTALL_LIBDIR} # XXX: change LIBDIR to BINDIR?
    )
endif()

# UI tools
# XXX: move to Qt5
if (QT4_FOUND AND NOT MSVC)
    set(QT_USE_QTOPENGL TRUE)

    set(SRC_FILES
        "tools/main.cpp"
        "tools/configdialog.h"
        "tools/configdialog.cpp"
    )

    group_sources(SRC_FILES)

    add_executable(nvcompressui MACOSX_BUNDLE ${SRC_FILES} ${UICS} ${MOCS})

    target_include_directories(${PROJECT_NAME}
        PRIVATE
        ${CMAKE_CURRENT_BINARY_DIR} # XXX: why?
        ${QT_INCLUDE_DIR} # XXX: check if variable is correct
    )

    target_link_libraries(nvcompressui
        PRIVATE
        nvtt
        ${QT_QTCORE_LIBRARY} # XXX: check if variable is correct
        ${QT_QTGUI_LIBRARY} # XXX: check if variable is correct
        ${QT_QTOPENGL_LIBRARY} # XXX: check if variable is correct
    )

    QT4_WRAP_UI(UICS tools/configdialog.ui)
    QT4_WRAP_CPP(MOCS tools/configdialog.h)
    #QT4_ADD_RESOURCES(RCCS tools/configdialog.rc)
endif()
