add_library(xrMaterialSystem SHARED)

target_sources_grouped(
    TARGET xrMaterialSystem
    NAME "Engine\\Game API\\MaterialSystem"
    FILES
    GameMtlLib.cpp
    GameMtlLib.h
    GameMtlLib_Engine.cpp
)

# XXX: This is only used in utils, should be moved somewhere to utils
#target_sources_grouped(
#    TARGET xrMaterialSystem
#    NAME ""
#    FILES
#    xrLoadSurface.cpp
#)

target_include_directories(xrMaterialSystem
    PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}"
    "${CMAKE_SOURCE_DIR}/src"
)

target_link_libraries(xrMaterialSystem
    PUBLIC
    xrMiscMath

    PRIVATE
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
