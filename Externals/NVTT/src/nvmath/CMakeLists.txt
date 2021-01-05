PROJECT(nvmath)

set(NVMATH_SHARED OFF)

set(SRC_FILES
    "nvmath.h"
    "Vector.h"
    "Matrix.h"
    "Quaternion.h"
    "Box.h"
    "Color.h"
    "Montecarlo.h"
    "Montecarlo.cpp"
    "Random.h"
    "Random.cpp"
    "SphericalHarmonic.h"
    "SphericalHarmonic.cpp"
    "Basis.h"
    "Basis.cpp"
    "Triangle.h"
    "Triangle.cpp"
    "TriBox.cpp"
)

group_sources(SRC_FILES)

if (NVMATH_SHARED)
    add_library(${PROJECT_NAME} SHARED ${SRC_FILES})
else()
    add_library(${PROJECT_NAME} STATIC ${SRC_FILES})
endif()

target_include_directories(${PROJECT_NAME}
    PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/Externals/NVTT/src
    ${CMAKE_SOURCE_DIR}/Externals/NVTT/include
)

target_link_libraries(${PROJECT_NAME}
    PRIVATE
    nvcore
)

target_compile_definitions(${PROJECT_NAME}
    PRIVATE
    -DNVCORE_EXPORTS
    $<$<BOOL:NVMATH_SHARED>:-DNVMATH_SHARED=1>
)

install(TARGETS ${PROJECT_NAME} LIBRARY
    DESTINATION ${CMAKE_INSTALL_LIBDIR}
    PERMISSIONS OWNER_READ OWNER_WRITE GROUP_READ WORLD_READ
)
