#pragma once

#include <vulkan/vulkan.h>

#include <cstddef>
#include <memory>
#include <string>
#include <unordered_map>

namespace xray::render::vulkan
{
struct ShaderModuleDispatch
{
    PFN_vkCreateShaderModule create{};
    PFN_vkDestroyShaderModule destroy{};
};

// Reject malformed binaries before creating a Vulkan shader module. SPIR-V
// execution model 0 is vertex, 4 is fragment.
bool has_spirv_entry(const void* bytes, size_t size, uint32_t stage, const char* entry);

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

// Device-scoped SPIR-V modules. Call clear only after submitted frames have
// completed; callers keep borrowed pointers until the next level/reload.
class ShaderModuleCache
{
public:
    ShaderModule* find(const std::string& key) const;
    bool load(const std::string& key, VkDevice device, const ShaderModuleDispatch& dispatch,
        const void* bytes, size_t size, uint32_t stage, const char* entry,
        ShaderModule*& result, std::string& error);
    void clear() { modules_.clear(); }
    size_t size() const { return modules_.size(); }

private:
    std::unordered_map<std::string, std::unique_ptr<ShaderModule>> modules_;
};
}
