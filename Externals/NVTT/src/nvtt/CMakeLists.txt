add_subdirectory(squish)

option(NVTT_SHARED "Build nvtt as shared library" OFF)
option(NVTT_BUILD_TEST_EXECUTABLES "Build nvtt test executables" OFF)
option(NVTT_BUILD_UI_TOOLS "Build nvtt UI tools" OFF)

if (NVTT_SHARED)
    add_library(nvtt SHARED)

    target_compile_definitions(nvtt
        PRIVATE
        NVTT_SHARED
    )
else()
    add_library(nvtt STATIC)
endif()

target_sources(nvtt PRIVATE
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
    cuda/CudaCompressDXT.cpp
)

if (CUDA_FOUND)
    wrap_cuda(CUDA_SRCS cuda/CompressKernel.cu)
    list(APPEND SRC_FILES ${CUDA_SRCS})
endif()

target_include_directories(nvtt
    PRIVATE
    $<$<BOOL:${CUDA_FOUND}>:${CUDA_INCLUDE_PATH}>
)

target_link_libraries(nvtt
    PUBLIC
    nvcore
    nvmath
    nvimage
    PRIVATE
    squish
    $<$<BOOL:${CUDA_FOUND}>:${CUDA_LIBRARIES}>
)

# TODO use configure_file for nvconfig.h

target_compile_definitions(nvtt
    PRIVATE
    NVTT_EXPORTS
    HAVE_SIGNAL_H
    HAVE_EXECINFO_H
    $<$<BOOL:${CUDA_FOUND}>:HAVE_CUDA>
)

set_target_properties(nvtt PROPERTIES
    UNITY_BUILD OFF
)

install(TARGETS nvtt LIBRARY
    DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

install(FILES nvtt.h DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/nvtt")

if (NVTT_TEST_EXECUTABLES)
    add_executable(nvcompress tools/compress.cpp tools/cmdline.h)
    target_link_libraries(nvcompress nvtt)

    add_executable(nvdecompress tools/decompress.cpp tools/cmdline.h)
    target_link_libraries(nvdecompress nvmath nvimage)

    add_executable(nvddsinfo tools/ddsinfo.cpp tools/cmdline.h)
    target_link_libraries(nvddsinfo nvmath nvimage)

    add_executable(nvimgdiff tools/imgdiff.cpp tools/cmdline.h)
    target_link_libraries(nvimgdiff nvmath nvimage)

    add_executable(nvassemble tools/assemble.cpp tools/cmdline.h)
    target_link_libraries(nvassemble nvmath nvimage)

    add_executable(filtertest tests/filtertest.cpp tools/cmdline.h)
    target_link_libraries(filtertest nvmath nvimage)

    add_executable(nvzoom tools/resize.cpp tools/cmdline.h)
    target_link_libraries(nvzoom nvmath nvimage)

    install(TARGETS nvcompress nvdecompress nvddsinfo nvimgdiff nvassemble nvzoom
        DESTINATION "${CMAKE_INSTALL_BINDIR}"
    )
endif()

if (NVTT_BUILD_UI_TOOLS)
    find_package(Qt5 COMPONENTS Core Widgets OpenGL)

    if (Qt5Core_FOUND AND Qt5Gui_FOUND AND Qt5OpenGL_FOUND)
        set(CMAKE_AUTOMOC ON)
        set(CMAKE_AUTOUIC ON)

        add_executable(nvcompressui)

        target_sources(nvcompressui PRIVATE
            tools/main.cpp
            tools/configdialog.h
            tools/configdialog.cpp
            tools/configdialog.ui
        )

        target_link_libraries(nvcompressui
            PRIVATE
            nvtt
            Qt5::Core
            Qt5::Widgets
            Qt5::OpenGL
        )
    endif()
endif()
