add_library(xrAnimation STATIC)

target_sources_grouped(
    TARGET xrAnimation
    NAME "Core"
    FILES
    stdafx.h
    stdafx.cpp
)

target_sources_grouped(
    TARGET xrAnimation
    NAME "Bundle"
    FILES
    ExtendedBoneMetadata.h
    OzzBundle.h
    OzzBundle.cpp
    OzzConversion.h
)

target_sources_grouped(
    TARGET xrAnimation
    NAME "Converters"
    FILES
    LegacyChunkIO.h
    LegacyMotionSerialization.h
    LegacyOgfConverter.h
    LegacyOgfConverter.cpp
    LegacyOmfConverter.h
    LegacyOmfConverter.cpp
    StartupConversionInventory.h
    StartupConversionInventory.cpp
)

target_sources(xrAnimation PRIVATE
    "${CMAKE_SOURCE_DIR}/Externals/ozz-animation/samples/framework/mesh.cc"
)
set_source_files_properties(
    "${CMAKE_SOURCE_DIR}/Externals/ozz-animation/samples/framework/mesh.cc"
    PROPERTIES SKIP_PRECOMPILE_HEADERS ON
)

target_include_directories(xrAnimation
    PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}"
    "${CMAKE_CURRENT_SOURCE_DIR}/.."
    "${CMAKE_SOURCE_DIR}/Externals/ozz-animation/samples"
)

target_link_libraries(xrAnimation
    PUBLIC
    xrCore
    xrAPI
    ozz_animation
    ozz_animation_offline
    ozz_base
)

set_target_properties(xrAnimation PROPERTIES PREFIX "")

target_precompile_headers(xrAnimation PRIVATE stdafx.h)

add_subdirectory(tools)
