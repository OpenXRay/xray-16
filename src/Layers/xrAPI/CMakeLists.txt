add_library(xrAPI SHARED)

target_sources(xrAPI PRIVATE
    stdafx.h
    stdafx.cpp
    xrAPI.cpp
)

target_include_directories(xrAPI
    PRIVATE
    "${CMAKE_SOURCE_DIR}/src"
)

target_compile_definitions(xrAPI
    PRIVATE
    XRAPI_EXPORTS
    _USRDLL
)

set_target_properties(xrAPI PROPERTIES
    PREFIX ""
)

target_precompile_headers(xrAPI
    PRIVATE
    stdafx.h
)

install(TARGETS xrAPI LIBRARY
    DESTINATION "${CMAKE_INSTALL_LIBDIR}"
)
