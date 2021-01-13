PROJECT(squish)

set(SRC_FILES
    "fastclusterfit.cpp"
    "fastclusterfit.h"
    "weightedclusterfit.cpp"
    "weightedclusterfit.h"
    "colourblock.cpp"
    "colourblock.h"
    "colourfit.cpp"
    "colourfit.h"
    "colourset.cpp"
    "colourset.h"
    "config.h"
    "maths.cpp"
    "maths.h"
    "simd.h"
    "simd_sse.h"
    "simd_ve.h"
)

group_sources(SRC_FILES)

add_library(${PROJECT_NAME} STATIC ${SRC_FILES})

target_include_directories(${PROJECT_NAME}
    PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_compile_definitions(${PROJECT_NAME}
    PRIVATE
    $<$<BOOL:CMAKE_COMPILER_IS_GNUCXX>:-fPIC>
)
