include_guard()

if (NOT WIN32)
    return()
endif()

set(XRAY_SDK_DIR "${CMAKE_SOURCE_DIR}/sdk")
set(XRAY_SDK_LIB_DIR "${XRAY_SDK_DIR}/libraries/x64")
set(XRAY_SDK_INC_DIR "${XRAY_SDK_DIR}/include")

include_directories(
    "${XRAY_SDK_INC_DIR}"
    "${XRAY_SDK_INC_DIR}/AL"
    "${CMAKE_SOURCE_DIR}/Externals"
)

add_library(OpenAL::OpenAL STATIC IMPORTED GLOBAL)
set_target_properties(OpenAL::OpenAL PROPERTIES
    IMPORTED_LOCATION "${XRAY_SDK_LIB_DIR}/OpenAL32.lib"
    INTERFACE_INCLUDE_DIRECTORIES "${XRAY_SDK_INC_DIR}"
)

add_library(Ogg::Ogg STATIC IMPORTED GLOBAL)
set_target_properties(Ogg::Ogg PROPERTIES
    IMPORTED_LOCATION "${XRAY_SDK_LIB_DIR}/libogg_static.lib"
    INTERFACE_INCLUDE_DIRECTORIES "${XRAY_SDK_INC_DIR}"
)

add_library(Vorbis::Vorbis STATIC IMPORTED GLOBAL)
set_target_properties(Vorbis::Vorbis PROPERTIES
    IMPORTED_LOCATION "${XRAY_SDK_LIB_DIR}/libvorbis_static.lib"
    INTERFACE_INCLUDE_DIRECTORIES "${XRAY_SDK_INC_DIR}"
    INTERFACE_LINK_LIBRARIES "Ogg::Ogg"
)

add_library(Vorbis::VorbisFile STATIC IMPORTED GLOBAL)
set_target_properties(Vorbis::VorbisFile PROPERTIES
    IMPORTED_LOCATION "${XRAY_SDK_LIB_DIR}/libvorbisfile.lib"
    INTERFACE_INCLUDE_DIRECTORIES "${XRAY_SDK_INC_DIR}"
    INTERFACE_LINK_LIBRARIES "Vorbis::Vorbis"
)

add_library(Theora::Theora STATIC IMPORTED GLOBAL)
set_target_properties(Theora::Theora PROPERTIES
    IMPORTED_LOCATION "${XRAY_SDK_LIB_DIR}/libtheora_static.lib"
    INTERFACE_INCLUDE_DIRECTORIES "${XRAY_SDK_INC_DIR}"
    INTERFACE_LINK_LIBRARIES "Ogg::Ogg"
)

add_library(JPEG::JPEG STATIC IMPORTED GLOBAL)
set_target_properties(JPEG::JPEG PROPERTIES
    IMPORTED_LOCATION "${XRAY_SDK_LIB_DIR}/jpeg-static.lib"
    INTERFACE_INCLUDE_DIRECTORIES "${XRAY_SDK_INC_DIR}"
)
set(JPEG_FOUND TRUE CACHE BOOL "libjpeg provided via sdk/" FORCE)

add_library(xray_ansel UNKNOWN IMPORTED GLOBAL)
set_target_properties(xray_ansel PROPERTIES
    IMPORTED_LOCATION "${XRAY_SDK_LIB_DIR}/AnselSDK64.lib"
    INTERFACE_INCLUDE_DIRECTORIES "${XRAY_SDK_INC_DIR}"
)

add_library(xray_discord UNKNOWN IMPORTED GLOBAL)
set_target_properties(xray_discord PROPERTIES
    IMPORTED_LOCATION "${XRAY_SDK_LIB_DIR}/discord_game_sdk.dll.lib"
    INTERFACE_INCLUDE_DIRECTORIES "${XRAY_SDK_INC_DIR}"
)

add_library(LZO::LZO STATIC IMPORTED GLOBAL)
set_target_properties(LZO::LZO PROPERTIES
    IMPORTED_LOCATION "${XRAY_SDK_LIB_DIR}/lzo.lib"
    INTERFACE_INCLUDE_DIRECTORIES "${XRAY_SDK_INC_DIR}"
)

add_library(xray_mimalloc STATIC IMPORTED GLOBAL)
set_target_properties(xray_mimalloc PROPERTIES
    IMPORTED_LOCATION "${XRAY_SDK_LIB_DIR}/mimalloc-static.lib"
    IMPORTED_LOCATION_DEBUG "${XRAY_SDK_LIB_DIR}/mimalloc-static-debug.lib"
    INTERFACE_INCLUDE_DIRECTORIES "${XRAY_SDK_INC_DIR}/mimalloc"
)
set(mimalloc_FOUND TRUE CACHE BOOL "mimalloc provided via sdk/" FORCE)

add_library(xray_bugtrap STATIC IMPORTED GLOBAL)
set_target_properties(xray_bugtrap PROPERTIES
    IMPORTED_LOCATION "${XRAY_SDK_LIB_DIR}/BugTrap.lib"
    INTERFACE_INCLUDE_DIRECTORIES "${XRAY_SDK_INC_DIR}"
)

set(SDL2_DIR "${CMAKE_SOURCE_DIR}/src/packages/sdl2.nuget.2.32.4/build/native")
add_library(SDL2::SDL2 SHARED IMPORTED GLOBAL)
set_target_properties(SDL2::SDL2 PROPERTIES
    IMPORTED_LOCATION "${SDL2_DIR}/bin/x64/SDL2.dll"
    IMPORTED_IMPLIB "${SDL2_DIR}/lib/x64/SDL2.lib"
    INTERFACE_INCLUDE_DIRECTORIES "${SDL2_DIR}/include"
)
