add_library(xrGameMtlLib SHARED)

target_sources_grouped(
    TARGET xrGameMtlLib
    NAME "Engine\\Game API\\MaterialSystem"
    FILES
    GameMtlLib.cpp
    GameMtlLib.h
    GameMtlLib_Engine.cpp
)

# XXX: This is only used in utils, should be moved somewhere to utils
#target_sources_grouped(
#    TARGET xrGameMtlLib
#    NAME ""
#    FILES
#    xrLoadSurface.cpp
#)

target_include_directories(xrGameMtlLib
    PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}"
    "${CMAKE_SOURCE_DIR}/src"
    "${CMAKE_SOURCE_DIR}/Externals/imgui"
    "${CMAKE_SOURCE_DIR}/Externals/OpenAutomate/inc"
)

target_link_libraries(xrGameMtlLib
    PUBLIC
    xrMiscMath

    PRIVATE
    xrAPI
    xrCDB
    xrCore
    xrSound
    xrScriptEngine
    xrNetServer
    xrImGui
    ${OPENAL_LIBRARY}
    ${OGG_LIBRARIES}
    Theora::Theora
)

target_compile_definitions(xrGameMtlLib
    PRIVATE
    ENGINE_BUILD
    PURE_DYNAMIC_CAST
)

set_target_properties(xrGameMtlLib PROPERTIES
    PREFIX ""
)

target_precompile_headers(xrGameMtlLib
    PRIVATE
    stdafx.h
)

install(TARGETS xrGameMtlLib LIBRARY
    DESTINATION "${CMAKE_INSTALL_LIBDIR}"
)
