option(NVMATH_SHARED "Build nvmath as shared library" OFF)

if (NVMATH_SHARED)
    add_library(nvmath SHARED)

    target_compile_definitions(nvmath
        PRIVATE
        NVCORE_SHARED
    )
else()
    add_library(nvmath STATIC)
endif()

target_sources(nvmath PRIVATE
    nvmath.h
    Vector.h
    Matrix.h
    Quaternion.h
    Box.h
    Color.h
    Montecarlo.h
    Montecarlo.cpp
    Random.h
    Random.cpp
    SphericalHarmonic.h
    SphericalHarmonic.cpp
    Basis.h
    Basis.cpp
    Triangle.h
    Triangle.cpp
    TriBox.cpp
)

target_link_libraries(nvmath
    PUBLIC
    nvcore
)

target_compile_definitions(nvmath
    PRIVATE
    NVCORE_EXPORTS
)

install(TARGETS nvmath LIBRARY
    DESTINATION "${CMAKE_INSTALL_LIBDIR}"
)
