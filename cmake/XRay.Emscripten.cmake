include_guard()

option(XRAY_WEB_PROFILING "Keep wasm function names for profiling" OFF)

set(XRAY_WEB_TARGET_FLAGS
    -pthread
    -m64
    -msimd128
    -msse3
    -fwasm-exceptions
    -sUSE_SDL=2
    -sUSE_OGG=1
    -sUSE_VORBIS=1
    -sUSE_LIBJPEG=1
)
add_compile_options(${XRAY_WEB_TARGET_FLAGS})
add_link_options(${XRAY_WEB_TARGET_FLAGS})

set(CMAKE_EXECUTABLE_SUFFIX ".js")

foreach (lib SDL2::SDL2 Ogg::Ogg Vorbis::Vorbis Vorbis::VorbisFile JPEG::JPEG OpenAL::OpenAL)
    add_library(${lib} INTERFACE IMPORTED)
endforeach()
target_link_options(OpenAL::OpenAL INTERFACE -lopenal)
target_include_directories(OpenAL::OpenAL INTERFACE "${EMSCRIPTEN_SYSROOT}/include/AL")
set(JPEG_FOUND TRUE)

add_subdirectory("${XRAY_SOURCE_DIR}/Externals/web" "${CMAKE_BINARY_DIR}/Externals/web")

set(LUA_INCLUDE_DIR "${XRAY_WEB_LUA_INCLUDE_DIR}" CACHE PATH "" FORCE)
set(LUA_LIBRARY xrLua51 CACHE STRING "" FORCE)
set(LUA_MATH_LIBRARY m CACHE STRING "" FORCE)
add_library(Lua51 ALIAS xrLua51)

set(XRAY_WEB_EXECUTABLE_LINK_OPTIONS
    -sPROXY_TO_PTHREAD
    -sOFFSCREENCANVAS_SUPPORT
    "-sOFFSCREENCANVASES_TO_PTHREAD=#canvas"
    "-sPTHREAD_POOL_SIZE=navigator.hardwareConcurrency+4"
    -sWASMFS
    -sALLOW_MEMORY_GROWTH
    -sINITIAL_MEMORY=2GB
    -sMAXIMUM_MEMORY=16GB
    -sSTACK_SIZE=16MB
    -sDEFAULT_PTHREAD_STACK_SIZE=4MB
    -sMIN_WEBGL_VERSION=2
    -sMAX_WEBGL_VERSION=2
    -sGL_ENABLE_GET_PROC_ADDRESS
    -sMALLOC=mimalloc
    -sEXIT_RUNTIME=0
    -sENVIRONMENT=web,worker
    -sMODULARIZE
    -sEXPORT_ES6
    --emit-symbol-map
    $<$<BOOL:${XRAY_WEB_PROFILING}>:--profiling-funcs>
    "--pre-js=${XRAY_SOURCE_DIR}/misc/web/pre.js"
    $<$<CONFIG:Debug,Mixed>:-sASSERTIONS=1>
    $<$<CONFIG:Debug,Mixed>:-sGL_ASSERTIONS=1>
    $<$<CONFIG:Debug,Mixed>:-gsource-map>
)
