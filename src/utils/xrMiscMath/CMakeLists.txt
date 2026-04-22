add_library(xrMiscMath STATIC)

target_sources(xrMiscMath PRIVATE
    matrix.cpp
    quaternion.cpp
    vector.cpp
    vector3d_ext.cpp
    xrMiscMath.cpp
    pch.hpp
    pch.cpp
)

target_include_directories(xrMiscMath
    PRIVATE
    "${CMAKE_SOURCE_DIR}/src"
)

target_link_libraries(xrMiscMath
    PRIVATE
    sse2neon
)

set_target_properties(xrMiscMath PROPERTIES
    POSITION_INDEPENDENT_CODE ${BUILD_SHARED_LIBS}
)

target_precompile_headers(xrMiscMath
    PRIVATE
    pch.hpp
)
