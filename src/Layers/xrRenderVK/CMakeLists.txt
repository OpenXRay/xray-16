add_library(xrRenderVK STATIC)

target_sources(xrRenderVK PRIVATE
    DdsTexture.cpp
    DdsTexture.h
    ImageStateTracker.cpp
    ImageStateTracker.h
    TextureUpload.cpp
    TextureUpload.h
    VulkanHardware.cpp
    VulkanHardware.h
)

include("${CMAKE_SOURCE_DIR}/cmake/XRay.Gli.cmake")
xray_gli_include_dir(XRAY_GLI_INCLUDE_DIR)

target_include_directories(xrRenderVK
    PUBLIC
    "${CMAKE_SOURCE_DIR}/Externals/Vulkan-Headers/include"
    PRIVATE
    "${XRAY_GLI_INCLUDE_DIR}"
    "${CMAKE_SOURCE_DIR}/Externals/gli/external"
)

set_target_properties(xrRenderVK PROPERTIES
    PREFIX ""
)
