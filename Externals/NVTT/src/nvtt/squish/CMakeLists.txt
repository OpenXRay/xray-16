PROJECT(squish)

add_library(squish STATIC)

target_sources(squish PRIVATE
    fastclusterfit.cpp
    fastclusterfit.h
    weightedclusterfit.cpp
    weightedclusterfit.h
    colourblock.cpp
    colourblock.h
    colourfit.cpp
    colourfit.h
    colourset.cpp
    colourset.h
    config.h
    maths.cpp
    maths.h
    simd.h
    simd_sse.h
    simd_ve.h
)

target_include_directories(squish
    PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
)

set_property(TARGET squish PROPERTY
    POSITION_INDEPENDENT_CODE ON
)
