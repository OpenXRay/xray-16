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
    "${XRAY_SOURCE_DIR}/src"
)

set_target_properties(xrMiscMath PROPERTIES
    POSITION_INDEPENDENT_CODE ${BUILD_SHARED_LIBS}
)

target_precompile_headers(xrMiscMath
    PRIVATE
    pch.hpp
)
