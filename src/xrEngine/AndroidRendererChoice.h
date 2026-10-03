#pragma once

#include <cstring>

namespace xray::render
{
enum class AndroidRendererChoice { Configured, Vulkan, GLES, VulkanUnavailable };

inline AndroidRendererChoice choose_android_renderer(const char* params, bool vulkan_ready)
{
    if (!params) return AndroidRendererChoice::Configured;
    if (std::strstr(params, "-renderer-vulkan"))
        return vulkan_ready ? AndroidRendererChoice::Vulkan : AndroidRendererChoice::VulkanUnavailable;
    if (std::strstr(params, "-renderer-auto"))
        return vulkan_ready ? AndroidRendererChoice::Vulkan : AndroidRendererChoice::GLES;
    if (std::strstr(params, "-renderer-gles")) return AndroidRendererChoice::GLES;
    return AndroidRendererChoice::Configured;
}
}
