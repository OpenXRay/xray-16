#include "src/Layers/xrRenderVK/ShaderModule.h"

#include <cassert>
#include <cstdint>
#include <cstring>
#include <utility>

namespace
{
uint32_t create_count = 0;
uint32_t destroy_count = 0;

VkShaderModule test_module_handle()
{
#if VK_USE_64_BIT_PTR_DEFINES
    return reinterpret_cast<VkShaderModule>(static_cast<uintptr_t>(1));
#else
    return static_cast<VkShaderModule>(1);
#endif
}

VkResult VKAPI_CALL create_module(VkDevice, const VkShaderModuleCreateInfo* info,
    const VkAllocationCallbacks*, VkShaderModule* module)
{
    assert(info->codeSize >= 5 * sizeof(uint32_t));
    assert(info->pCode[0] == 0x07230203);
    *module = test_module_handle();
    ++create_count;
    return VK_SUCCESS;
}

void VKAPI_CALL destroy_module(VkDevice, VkShaderModule, const VkAllocationCallbacks*)
{
    ++destroy_count;
}
}

int main()
{
    const VkDevice device = reinterpret_cast<VkDevice>(static_cast<uintptr_t>(1));
    const xray::render::vulkan::ShaderModuleDispatch dispatch{create_module, destroy_module};
    const uint32_t code[] = {0x07230203, 0x00010000, 0, 1, 0};
    const uint32_t vertex_entry[] = {0x07230203, 0x00010000, 0, 2, 0,
        0x0005000f, 0, 1, 0x6e69616d, 0};
    assert(xray::render::vulkan::has_spirv_entry(vertex_entry, sizeof(vertex_entry), 0, "main"));
    assert(!xray::render::vulkan::has_spirv_entry(vertex_entry, sizeof(vertex_entry), 4, "main"));
    assert(!xray::render::vulkan::has_spirv_entry(vertex_entry, sizeof(vertex_entry), 0, "other"));
    uint32_t malformed[10]{};
    std::memcpy(malformed, vertex_entry, sizeof(vertex_entry));
    malformed[5] = 0x000b000f;
    assert(!xray::render::vulkan::has_spirv_entry(malformed, sizeof(malformed), 0, "main"));
    std::string error;
    xray::render::vulkan::ShaderModule module;

    assert(!module.initialize(device, dispatch, code, sizeof(code) - 1, error));
    assert(!module.handle());
    assert(module.initialize(device, dispatch, code, sizeof(code), error));
    assert(module.handle());
    assert(create_count == 1);

    uint8_t unaligned[sizeof(code) + 1]{};
    std::memcpy(unaligned + 1, code, sizeof(code));
    assert(module.initialize_bytes(device, dispatch, unaligned + 1, sizeof(code), error));
    assert(create_count == 2);
    assert(destroy_count == 1);

    xray::render::vulkan::ShaderModule moved(std::move(module));
    assert(!module.handle());
    assert(moved.handle());
    moved.destroy();
    assert(destroy_count == 2);

    xray::render::vulkan::ShaderModuleCache cache;
    xray::render::vulkan::ShaderModule* first = nullptr;
    assert(cache.load("vk/level_opaque.vs.spv", device, dispatch,
        vertex_entry, sizeof(vertex_entry), 0, "main", first, error));
    assert(first && cache.size() == 1 && create_count == 3);
    xray::render::vulkan::ShaderModule* again = nullptr;
    assert(cache.load("vk/level_opaque.vs.spv", device, dispatch,
        vertex_entry, sizeof(vertex_entry), 0, "main", again, error));
    assert(first == again && create_count == 3);
    assert(!cache.load("vk/invalid.ps.spv", device, dispatch,
        vertex_entry, sizeof(vertex_entry), 4, "main", again, error));
    assert(!again && cache.size() == 1 && create_count == 3);
    cache.clear();
    assert(destroy_count == 3 && !cache.size());
    assert(cache.load("vk/level_opaque.vs.spv", device, dispatch,
        vertex_entry, sizeof(vertex_entry), 0, "main", again, error));
    assert(again && create_count == 4);
    cache.clear();
    assert(destroy_count == 4);
}
