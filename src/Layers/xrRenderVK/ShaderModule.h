#pragma once

#include <vulkan/vulkan.h>

#include <cstddef>
#include <string>

namespace xray::render::vulkan
{
struct ShaderModuleDispatch
{
    PFN_vkCreateShaderModule create{};
    PFN_vkDestroyShaderModule destroy{};
};

class ShaderModule
{
public:
    ShaderModule() = default;
    ~ShaderModule();
    ShaderModule(const ShaderModule&) = delete;
    ShaderModule& operator=(const ShaderModule&) = delete;
    ShaderModule(ShaderModule&& other) noexcept;
    ShaderModule& operator=(ShaderModule&& other) noexcept;

    bool initialize(VkDevice device, const ShaderModuleDispatch& dispatch,
        const uint32_t* code, size_t size, std::string& error);
    // IReader::pointer() is byte-aligned; copy to aligned words before Vulkan.
    bool initialize_bytes(VkDevice device, const ShaderModuleDispatch& dispatch,
        const void* bytes, size_t size, std::string& error);
    void destroy();

    VkShaderModule handle() const { return m_module; }

private:
    VkDevice m_device = VK_NULL_HANDLE;
    VkShaderModule m_module = VK_NULL_HANDLE;
    PFN_vkDestroyShaderModule m_destroy{};
};
}
