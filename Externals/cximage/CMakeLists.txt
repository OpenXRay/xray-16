add_library(cximage STATIC)

target_sources(cximage PRIVATE
    xfile.h
    ximacfg.cpp
    ximacfg.h
    ximadef.h
    ximaenc.cpp
    ximaexif.cpp
    ximage.cpp
    ximage.h
    ximainfo.cpp
    ximaint.cpp
    ximaiter.h
    ximajpg.cpp
    ximajpg.h
    ximalpha.cpp
    ximalyr.cpp
    ximapal.cpp
    ximasel.cpp
    ximath.cpp
    ximath.h
    xiofile.h
    xiofile.cpp
    xmemfile.cpp
    xmemfile.h
)

target_include_directories(cximage
    PRIVATE
    "${CMAKE_SOURCE_DIR}/src"
)

target_link_libraries(cximage
    PRIVATE
    $<$<BOOL:${JPEG_FOUND}>:JPEG::JPEG>
)

target_compile_definitions(cximage
    PRIVATE
    CXIMAGE_BUILD
)

# XXX: Clang-9 internal error in function GetPixelColorInterpolated during optimization
if (CMAKE_CXX_COMPILER_ID MATCHES "Clang" AND (CMAKE_CXX_COMPILER_VERSION MATCHES "9.0.0" OR CMAKE_CXX_COMPILER_VERSION MATCHES "10.0.0"))
    set_property(SOURCE ximaint.cpp PROPERTY SKIP_UNITY_BUILD_INCLUSION TRUE)
endif()

set_target_properties(cximage PROPERTIES
    PREFIX ""
    POSITION_INDEPENDENT_CODE ON
)
