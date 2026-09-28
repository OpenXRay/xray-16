#pragma once

#if defined(XR_PLATFORM_ANDROID)

#include <string>

namespace AndroidVulkanSmoke
{
// Records and submits a Vulkan color render pass to an Android swapchain.
// This is an independent renderer smoke path, not a gameplay renderer.
bool Run(std::string& reason);
}

#endif
