add_library(xrRenderVK STATIC)

target_sources(xrRenderVK PRIVATE
    VulkanRendererModule.cpp
    BufferResource.cpp
    BufferResource.h
    BufferUpload.cpp
    BufferUpload.h
    DdsTexture.cpp
    DdsTexture.h
    DeferredPass.cpp
    DeferredPass.h
    DeferredShaderFactory.cpp
    DeferredShaderFactory.h
    DeferredShaders.h
    EngineTextureSource.cpp
    EngineTextureSource.h
    EngineLevelModels.cpp
    EngineLevelModels.h
    FrameContext.cpp
    FrameContext.h
    GameTextureFactory.cpp
    GameTextureFactory.h
    ImageStateTracker.cpp
    ImageStateTracker.h
    LevelModels.cpp
    LevelModels.h
    ScreenCopyPass.cpp
    ScreenCopyPass.h
    ScenePass.cpp
    ScenePass.h
    ShaderModule.cpp
    ShaderModule.h
    SmokeShaders.h
    SmokeTrianglePass.cpp
    SmokeTrianglePass.h
    TextureUpload.cpp
    TextureUpload.h
    VulkanHardware.cpp
    VulkanHardware.h
    VulkanWindowDevice.cpp
    VulkanWindowDevice.h
    VisualCatalog.cpp
    VisualCatalog.h
)

include("${CMAKE_SOURCE_DIR}/cmake/XRay.Gli.cmake")
xray_gli_include_dir(XRAY_GLI_INCLUDE_DIR)

target_include_directories(xrRenderVK
    PUBLIC
    "${CMAKE_SOURCE_DIR}/Externals/Vulkan-Headers/include"
    PRIVATE
    "${CMAKE_SOURCE_DIR}/src"
    "${CMAKE_SOURCE_DIR}/sdk/include"
    "${CMAKE_SOURCE_DIR}/Externals"
    "${CMAKE_SOURCE_DIR}/Externals/tracy/public"
    "${CMAKE_SOURCE_DIR}/Externals/imgui"
    "${XRAY_GLI_INCLUDE_DIR}"
    "${CMAKE_SOURCE_DIR}/Externals/gli/external"
)

target_link_libraries(xrRenderVK PRIVATE SDL2::SDL2)

set_target_properties(xrRenderVK PROPERTIES
    PREFIX ""
)
