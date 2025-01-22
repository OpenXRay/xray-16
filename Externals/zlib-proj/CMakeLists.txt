add_library(zlib STATIC)

add_library(zlib::zlib ALIAS zlib)

# TODO disable unity build?

target_include_directories(zlib
    PUBLIC
    #../zlib/include
    ${CMAKE_CURRENT_BINARY_DIR}
    PRIVATE
    ../zlib
)

target_compile_definitions(zlib
    PRIVATE
    _CRT_NONSTDC_NO_DEPRECATE
    _CRT_NONSTDC_NO_WARNINGS
    ZLIB_WINAPI
    WIN64
    #WIN32
)

configure_file(../zlib/zconf.h.cmakein ${CMAKE_CURRENT_BINARY_DIR}/zconf.h @ONLY)

target_sources(zlib
    PRIVATE
    ../zlib/adler32.c
    ../zlib/compress.c
    ../zlib/crc32.c
    ../zlib/crc32.h
    ../zlib/deflate.c
    ../zlib/gzclose.c
    ../zlib/gzlib.c
    ../zlib/gzread.c
    ../zlib/gzwrite.c
    ../zlib/infback.c
    ../zlib/inffast.c
    ../zlib/inffast.h
    ../zlib/inflate.c
    ../zlib/inflate.h
    ../zlib/inftrees.c
    ../zlib/inftrees.h
    ../zlib/contrib/minizip/ioapi.c
    ../zlib/trees.c
    ../zlib/trees.h
    ../zlib/uncompr.c
    ../zlib/contrib/minizip/unzip.c
    ../zlib/contrib/minizip/zip.c
    ../zlib/zutil.c
    ../zlib/zutil.h
)
