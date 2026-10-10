cmake_minimum_required(VERSION 3.23)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
include(PreProjectInit)

calculate_xray_build_id(XRAY_BUILD_ID)
project(OpenXRay
    DESCRIPTION "OpenXRay is an improved version of the X-Ray Engine, \
                the game engine used in the world-famous S.T.A.L.K.E.R. game series by GSC Game World."
    VERSION 1.6.02.${XRAY_BUILD_ID}
    HOMEPAGE_URL "https://github.com/OpenXRay/xray-16"
    LANGUAGES CXX C
)

if (APPLE)
    enable_language(OBJC)
    enable_language(OBJCXX)
endif()

option(BUILD_SHARED_LIBS "Build shared libraries"     ${BUILD_SHARED_LIBS_DEFAULT_VALUE})

option(XRAY_USE_LUAJIT   "Use LuaJIT"                 ON)
option(XRAY_USE_ASAN     "Use AddressSanitizer"       OFF)
option(XRAY_ENABLE_TRACY "Enable tracy profiler"      OFF)
option(XRAY_USE_AI_PBR   "Enable AI PBR texture converter (requires ONNX Runtime)" OFF)

include(XRay.Build)
include(XRay.Windows.Externals)
include(XRay.Packaging)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_compile_definitions(
    $<$<NOT:$<BOOL:${BUILD_SHARED_LIBS}>>:XRAY_STATIC_BUILD>
)

block()
    set(CMAKE_UNITY_BUILD OFF)
    add_subdirectory(Externals)
endblock()
add_subdirectory(src)
add_subdirectory(res)
add_subdirectory(misc)
