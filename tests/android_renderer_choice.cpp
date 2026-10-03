#include "src/xrEngine/AndroidRendererChoice.h"
#include <cassert>

int main()
{
    using xray::render::AndroidRendererChoice;
    using xray::render::choose_android_renderer;
    assert(choose_android_renderer("-renderer-vulkan", true) == AndroidRendererChoice::Vulkan);
    assert(choose_android_renderer("-renderer-vulkan", false) == AndroidRendererChoice::VulkanUnavailable);
    assert(choose_android_renderer("-renderer-auto", true) == AndroidRendererChoice::Vulkan);
    assert(choose_android_renderer("-renderer-auto", false) == AndroidRendererChoice::GLES);
    assert(choose_android_renderer("-renderer-gles", true) == AndroidRendererChoice::GLES);
    assert(choose_android_renderer("", false) == AndroidRendererChoice::Configured);
    assert(choose_android_renderer(nullptr, true) == AndroidRendererChoice::Configured);
}
