#pragma once

#include <cstring>
#include <cctype>
#include <string_view>

namespace xray::render
{
enum class AndroidRendererChoice
{
    Configured,
    Vulkan,
    GLES,
    VulkanUnavailable
};

inline bool has_renderer_option(const char *params, std::string_view option)
{
    if (!params)
        return false;
    const char *p = params;
    while (*p)
    {
        while (*p && std::isspace(static_cast<unsigned char>(*p)))
            ++p;
        if (!*p)
            break;
        const char quote = (*p == '"' || *p == '\'') ? *p++ : 0;
        const char *start = p;
        while (*p && (quote ? *p != quote : !std::isspace(static_cast<unsigned char>(*p))))
            ++p;
        if (std::string_view(start, p - start) == option)
            return true;
        if (quote && *p)
            ++p;
    }
    return false;
}

inline bool android_renderer_needs_vulkan_probe(const char *params)
{
    return has_renderer_option(params, "-renderer-vulkan") || has_renderer_option(params, "-renderer-auto");
}

inline bool android_native_splash_allowed(const char *params)
{
    // Android SDL's software splash can create an EGL-backed window before
    // renderer selection. Auto may choose Vulkan, so both requests skip it.
    return !android_renderer_needs_vulkan_probe(params) && !has_renderer_option(params, "-nosplash");
}

inline AndroidRendererChoice choose_android_renderer(const char *params, bool vulkan_ready)
{
    if (!params)
        return AndroidRendererChoice::Configured;
    if (has_renderer_option(params, "-renderer-vulkan"))
        return vulkan_ready ? AndroidRendererChoice::Vulkan : AndroidRendererChoice::VulkanUnavailable;
    if (has_renderer_option(params, "-renderer-auto"))
        return vulkan_ready ? AndroidRendererChoice::Vulkan : AndroidRendererChoice::GLES;
    if (has_renderer_option(params, "-renderer-gles"))
        return AndroidRendererChoice::GLES;
    return AndroidRendererChoice::Configured;
}
} // namespace xray::render
