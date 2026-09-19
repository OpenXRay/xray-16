add_library(xrAnimation)

target_sources_grouped(
    TARGET xrAnimation
    NAME "Core"
    FILES
    stdafx.h
    stdafx.cpp
    xrAnimation.h
)

target_sources_grouped(
    TARGET xrAnimation
    NAME "Runtime"
    FILES
    OzzMotionLibrary.h
    OzzMotionLibrary.cpp
    OzzSkeletonMirror.h
    OzzSkeletonMirror.cpp
    OzzPose.h
    OzzPose.cpp
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
    ozz_base
    PRIVATE
    ozz_animation_offline
)

set_target_properties(xrAnimation PROPERTIES PREFIX "")
target_compile_definitions(xrAnimation PRIVATE XRANIMATION_EXPORTS)

target_precompile_headers(xrAnimation PRIVATE stdafx.h)

if(BUILD_SHARED_LIBS)
    install(TARGETS xrAnimation LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}")
endif()
