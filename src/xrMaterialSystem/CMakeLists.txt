add_library(xrMaterialSystem SHARED)

target_sources_grouped(
    TARGET xrMaterialSystem
    NAME "Kernel"
    FILES
    GameMtlLib.cpp
    GameMtlLib.h
    GameMtlLib_Engine.cpp
)

target_include_directories(xrMaterialSystem
    PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}"
    "${CMAKE_SOURCE_DIR}/src"
)

target_link_libraries(xrMaterialSystem
    PUBLIC
    xrMiscMath

    PRIVATE
    xrAPI
    xrCore
)

target_compile_definitions(xrMaterialSystem
    PRIVATE
    XRMTL_LIB_EXPORTS
)

set_target_properties(xrMaterialSystem PROPERTIES
    PREFIX ""
)

target_precompile_headers(xrMaterialSystem
    PRIVATE
    stdafx.h
)

install(TARGETS xrMaterialSystem LIBRARY
    DESTINATION "${CMAKE_INSTALL_LIBDIR}"
)
