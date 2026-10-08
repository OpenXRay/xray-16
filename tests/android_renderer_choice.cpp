#include "src/xrEngine/AndroidRendererChoice.h"
#include <cassert>

int main()
{
    using xray::render::AndroidRendererChoice;
    using xray::render::choose_android_renderer;
    assert(choose_android_renderer("-renderer-vulkan", true) == AndroidRendererChoice::Vulkan);
    assert(choose_android_renderer("-renderer-vulkan", false) == AndroidRendererChoice::VulkanUnavailable);
    assert(choose_android_renderer("-renderer-auto", true) == AndroidRendererChoice::GLES);
    assert(choose_android_renderer("-renderer-auto", false) == AndroidRendererChoice::GLES);
    assert(choose_android_renderer("-renderer-gles", true) == AndroidRendererChoice::GLES);
    assert(choose_android_renderer("", false) == AndroidRendererChoice::Configured);
    assert(choose_android_renderer(nullptr, true) == AndroidRendererChoice::Configured);
    assert(choose_android_renderer("-renderer-vulkan-smoke", true) == AndroidRendererChoice::Configured);
    assert(choose_android_renderer("-renderer-vulkan-invalid", true) == AndroidRendererChoice::Configured);
    assert(choose_android_renderer("\"path containing -renderer-vulkan value\"", true) == AndroidRendererChoice::Configured);
    assert(choose_android_renderer("-unique_logs\t-renderer-vulkan\n-nosplash", true) == AndroidRendererChoice::Vulkan);
    assert(!xray::render::android_native_splash_allowed("-renderer-vulkan"));
    assert(xray::render::android_native_splash_allowed("-renderer-auto"));
    assert(!xray::render::android_native_splash_allowed("-renderer-gles -nosplash"));
    assert(xray::render::android_native_splash_allowed("-renderer-gles"));
}
