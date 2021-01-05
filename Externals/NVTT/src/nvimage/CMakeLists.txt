PROJECT(nvimage)

set(NVIMAGE_SHARED OFF)

set(SRC_FILES
    "nvimage.h"
    "FloatImage.h"
    "FloatImage.cpp"
    "Filter.h"
    "Filter.cpp"
    "Image.h"
    "Image.cpp"
    "ImageIO.h"
    "ImageIO.cpp"
    "ColorBlock.h"
    "ColorBlock.cpp"
    "BlockDXT.h"
    "BlockDXT.cpp"
    "HoleFilling.h"
    "HoleFilling.cpp"
    "DirectDrawSurface.h"
    "DirectDrawSurface.cpp"
    "Quantize.h"
    "Quantize.cpp"
    "NormalMap.h"
    "NormalMap.cpp"
    "NormalMipmap.h"
    "NormalMipmap.cpp"
    "PsdFile.h"
    "TgaFile.h"
)

group_sources(SRC_FILES)

if (NVIMAGE_SHARED)
    add_library(${PROJECT_NAME} SHARED ${SRC_FILES})
else()
    add_library(${PROJECT_NAME} STATIC ${SRC_FILES})
endif()

target_include_directories(${PROJECT_NAME}
    PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/Externals/NVTT/src
    ${CMAKE_SOURCE_DIR}/Externals/NVTT/include
    $<$<BOOL:PNG_FOUND>:${PNG_INCLUDE_DIR}>
    $<$<BOOL:JPEG_FOUND>:${JPEG_INCLUDE_DIR}>
    $<$<BOOL:TIFF_FOUND>:${TIFF_INCLUDE_DIR}>
    $<$<BOOL:OPENEXR_FOUND>:${OPENEXR_INCLUDE_PATHS}>
)

target_link_libraries(${PROJECT_NAME}
    PRIVATE
    nvcore
    nvmath
    posh
    $<$<BOOL:PNG_FOUND>:${PNG_LIBRARIES}>
    $<$<BOOL:JPEG_FOUND>:${JPEG_LIBRARIES}>
    $<$<BOOL:TIFF_FOUND>:${TIFF_LIBRARIES}>
    $<$<BOOL:OPENEXR_FOUND>:${OPENEXR_LIBRARIES}>
)

target_compile_definitions(${PROJECT_NAME}
    PRIVATE
    -DNVIMAGE_EXPORTS
    $<$<BOOL:NVIMAGE_SHARED>:-DNVIMAGE_SHARED=1>
)

install(TARGETS ${PROJECT_NAME} LIBRARY
    DESTINATION ${CMAKE_INSTALL_LIBDIR}
    PERMISSIONS OWNER_READ OWNER_WRITE GROUP_READ WORLD_READ
)
