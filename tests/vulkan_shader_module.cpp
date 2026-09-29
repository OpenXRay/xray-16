#include "src/Layers/xrRenderVK/ShaderModule.h"

#include <cassert>
#include <cstdint>
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
    assert(info->codeSize == 5 * sizeof(uint32_t));
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
    std::string error;
    xray::render::vulkan::ShaderModule module;

    assert(!module.initialize(device, dispatch, code, sizeof(code) - 1, error));
    assert(!module.handle());
    assert(module.initialize(device, dispatch, code, sizeof(code), error));
    assert(module.handle());
    assert(create_count == 1);

    xray::render::vulkan::ShaderModule moved(std::move(module));
    assert(!module.handle());
    assert(moved.handle());
    moved.destroy();
    assert(destroy_count == 1);
}
