#pragma once

#if defined(XR_PLATFORM_ANDROID)

#include <string>

namespace AndroidVulkanSmoke
{
// Draws and reads back a Vulkan triangle without game assets in smoke mode.
// This is an independent diagnostic path, not a gameplay renderer.
bool Run(std::string& reason);
}

#endif
