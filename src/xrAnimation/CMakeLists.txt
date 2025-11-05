cmake_minimum_required(VERSION 3.8)

project(xrAnimation)

# ozz-animation is included as subdirectory, no need to find_package

# Include directories
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${CMAKE_CURRENT_SOURCE_DIR}/../Include
    ${CMAKE_CURRENT_SOURCE_DIR}/../xrCore
    ${CMAKE_CURRENT_SOURCE_DIR}/../xrEngine
    ${CMAKE_CURRENT_SOURCE_DIR}/../Common
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Externals/ozz-animation/include
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Externals/ozz-animation/samples
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Externals/imgui
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Externals/imgui-proj
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Externals/entt/single_include
)

# Source files
set(SOURCES
    stdafx.cpp
    stdafx.h
    xrAnimation.cpp
    xrAnimation.h
    OzzBundle.cpp
    OzzBundle.h
    LegacyOgfConverter.cpp
    LegacyOgfConverter.h
    LegacyOmfConverter.cpp
    LegacyOmfConverter.h
    OzzKinematicsCore.cpp
    OzzKinematicsCore.h
    OzzKinematics.cpp
    OzzKinematics.h
    OzzKinematicsAnimated.cpp
    OzzKinematicsAnimated.h
    OzzSharedMotions.cpp
    OzzSharedMotions.hpp
    StartupConversionInventory.cpp
    StartupConversionInventory.h
    AnimationECS_Components.h
    AnimationECS_Systems.h
    AnimationECS_Registry.h
    AnimationECS_ParallelSystems.h
    AnimationECS_Performance.h
    AnimationECS_IK.h
    AnimationECS_IK.cpp
    AnimationECS_DebugRender.h
    AnimationECS_DebugRender.cpp
    IDebugDrawContext.h
    ../../Externals/ozz-animation/samples/framework/mesh.cc
    ../../Externals/ozz-animation/samples/framework/utils.cc
)

# Create static library
add_library(xrAnimation STATIC ${SOURCES})

target_include_directories(xrAnimation
    PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${CMAKE_CURRENT_SOURCE_DIR}/../Include
    ${CMAKE_CURRENT_SOURCE_DIR}/../xrCore
    ${CMAKE_CURRENT_SOURCE_DIR}/../xrEngine
    ${CMAKE_CURRENT_SOURCE_DIR}/../Common
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Externals/ozz-animation/include
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Externals/ozz-animation/samples
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Externals/imgui
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Externals/imgui-proj
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Externals/entt/single_include
)

set_source_files_properties(
    ../../Externals/ozz-animation/samples/framework/mesh.cc
    ../../Externals/ozz-animation/samples/framework/utils.cc
    PROPERTIES
        SKIP_PRECOMPILE_HEADERS ON
)

# Link libraries
target_link_libraries(xrAnimation
    PRIVATE
    xrCore
    ozz_animation_offline
    ozz_animation
    ozz_base
    EnTT::EnTT
)

# Set target properties
set_target_properties(xrAnimation PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
    POSITION_INDEPENDENT_CODE ON
    FOLDER "xrAnimation"
)

# Compiler-specific options
if(MSVC)
    target_compile_options(xrAnimation PRIVATE
        /MP  # Multi-processor compilation
        /W3  # Warning level 3
        /wd4996  # Disable deprecated function warnings
    )

    # Use precompiled headers
    target_precompile_headers(xrAnimation PRIVATE stdafx.h)
endif()

# Install
install(TARGETS xrAnimation
    ARCHIVE DESTINATION lib
    LIBRARY DESTINATION lib
    RUNTIME DESTINATION bin
)

# Add tests
add_subdirectory(tests)

# Add tools
add_subdirectory(tools)
