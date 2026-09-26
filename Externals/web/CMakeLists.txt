include(FetchContent)

FetchContent_Declare(lzo
    URL https://www.oberhumer.com/opensource/lzo/download/lzo-2.10.tar.gz
    URL_HASH SHA256=c0f892943208266f9b6543b3ae308fab6284c5c90e627931446fb49b4221a072
    SOURCE_SUBDIR cmake-disabled
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE
)
FetchContent_Declare(theora
    URL https://downloads.xiph.org/releases/theora/libtheora-1.1.1.tar.bz2
    URL_HASH SHA256=b6ae1ee2fa3d42ac489287d3ec34c5885730b1296f0801ae577a35193d3affbc
    SOURCE_SUBDIR cmake-disabled
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE
)
FetchContent_Declare(lua
    URL https://www.lua.org/ftp/lua-5.1.5.tar.gz
    URL_HASH SHA256=2640fc56a795f29d28ef15e13c34a47e223960b0240e8cb0a82d9b0738695333
    SOURCE_SUBDIR cmake-disabled
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE
)
FetchContent_MakeAvailable(lzo theora lua)

add_compile_options(-w)
set(CMAKE_UNITY_BUILD OFF)

file(GLOB LZO_SOURCES "${lzo_SOURCE_DIR}/src/*.c")
add_library(xrLzo STATIC ${LZO_SOURCES})
target_include_directories(xrLzo PUBLIC "${lzo_SOURCE_DIR}/include")
add_library(LZO::LZO ALIAS xrLzo)

set(THEORA_DECODER_SOURCES
    apiwrapper.c bitpack.c decapiwrapper.c decinfo.c decode.c dequant.c
    fragment.c huffdec.c idct.c info.c internal.c quant.c state.c
)
list(TRANSFORM THEORA_DECODER_SOURCES PREPEND "${theora_SOURCE_DIR}/lib/")
add_library(xrTheoraDec STATIC ${THEORA_DECODER_SOURCES})
target_include_directories(xrTheoraDec
    PUBLIC "${theora_SOURCE_DIR}/include"
    PRIVATE "${theora_SOURCE_DIR}/lib"
)
add_library(Theora::Theora ALIAS xrTheoraDec)

file(GLOB LUA_SOURCES "${lua_SOURCE_DIR}/src/*.c")
list(FILTER LUA_SOURCES EXCLUDE REGEX "/(lua|luac|print)\\.c$")
add_library(xrLua51 STATIC ${LUA_SOURCES})
target_include_directories(xrLua51 PUBLIC "${lua_SOURCE_DIR}/src" "${lua_SOURCE_DIR}/etc")
set(XRAY_WEB_LUA_INCLUDE_DIR "${lua_SOURCE_DIR}/src" PARENT_SCOPE)
