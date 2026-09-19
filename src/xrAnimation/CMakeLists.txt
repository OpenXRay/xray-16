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
    NAME "Runtime"
    FILES
    OzzMotionLibrary.h
    OzzMotionLibrary.cpp
    OzzSkeletonMirror.h
    OzzSkeletonMirror.cpp
)

target_sources_grouped(
    TARGET xrAnimation
    NAME "Legacy"
    FILES
    LegacyChunkIO.h
    LegacyOgfSkeleton.h
    LegacyOgfSkeleton.cpp
    LegacyOmfConverter.h
    LegacyOmfConverter.cpp
    StartupConversionInventory.h
    StartupConversionInventory.cpp
)

target_include_directories(xrAnimation
    PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}"
    "${CMAKE_CURRENT_SOURCE_DIR}/.."
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
