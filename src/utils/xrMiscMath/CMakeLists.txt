project(xrMiscMath)

set(SRC_FILES
    "cylinder.cpp"
    "matrix.cpp"
    "quaternion.cpp"
    "vector.cpp"
    "vector3d_ext.cpp"
    "xrMiscMath.cpp"
    "pch.hpp"
    "pch.cpp"
)

group_sources(SRC_FILES)

add_library(${PROJECT_NAME} STATIC ${SRC_FILES})

target_include_directories(${PROJECT_NAME}
    PUBLIC
    ${CMAKE_SOURCE_DIR}/src
)

set_target_properties(${PROJECT_NAME} PROPERTIES
    POSITION_INDEPENDENT_CODE ON
)

target_precompile_headers(${PROJECT_NAME}
    PRIVATE
    "pch.hpp"
)
