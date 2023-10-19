option(NVIMAGE_SHARED "Build nvimage as shared library" OFF)

if (NVIMAGE_SHARED)
    add_library(nvimage SHARED)

    target_compile_definitions(nvimage
        PRIVATE
        NVIMAGE_SHARED=1
    )
else()
    add_library(nvimage STATIC)
endif()

target_sources(nvimage PRIVATE
    nvimage.h
    FloatImage.h
    FloatImage.cpp
    Filter.h
    Filter.cpp
    Image.h
    Image.cpp
    ImageIO.h
    ImageIO.cpp
    ColorBlock.h
    ColorBlock.cpp
    BlockDXT.h
    BlockDXT.cpp
    HoleFilling.h
    HoleFilling.cpp
    DirectDrawSurface.h
    DirectDrawSurface.cpp
    Quantize.h
    Quantize.cpp
    NormalMap.h
    NormalMap.cpp
    NormalMipmap.h
    NormalMipmap.cpp
    PsdFile.h
    TgaFile.h
)

target_link_libraries(nvimage
    PUBLIC
    nvcore
    nvmath
    PRIVATE
    posh
    $<$<BOOL:${PNG_FOUND}>:PNG::PNG>
    $<$<BOOL:${JPEG_FOUND}>:JPEG::JPEG>
    $<$<BOOL:${TIFF_FOUND}>:TIFF::TIFF>
    $<$<BOOL:${OPENEXR_FOUND}>:OpenEXR::OpenEXR>
)

target_compile_definitions(nvimage
    PRIVATE
    NVIMAGE_EXPORTS
)

set_property(TARGET nvimage PROPERTY
    POSITION_INDEPENDENT_CODE ON
)

install(TARGETS nvimage LIBRARY
    DESTINATION "${CMAKE_INSTALL_LIBDIR}"
)
