#include "ShaderModule.h"

#include <cstring>
#include <utility>
#include <vector>

namespace xray::render::vulkan
{
namespace
{
constexpr uint32_t SpirvMagic = 0x07230203;
}

ShaderModule::~ShaderModule()
{
    destroy();
}

ShaderModule::ShaderModule(ShaderModule&& other) noexcept
{
    *this = std::move(other);
}

ShaderModule& ShaderModule::operator=(ShaderModule&& other) noexcept
{
    if (this != &other)
    {
        destroy();
        m_device = other.m_device;
        m_module = other.m_module;
        m_destroy = other.m_destroy;
        other.m_device = VK_NULL_HANDLE;
        other.m_module = VK_NULL_HANDLE;
        other.m_destroy = nullptr;
    }
    return *this;
}

bool ShaderModule::initialize(VkDevice device, const ShaderModuleDispatch& dispatch,
    const uint32_t* code, size_t size, std::string& error)
{
    destroy();
    if (!device || !dispatch.create || !dispatch.destroy || !code || size < 5 * sizeof(uint32_t) ||
        size % sizeof(uint32_t) != 0 || code[0] != SpirvMagic)
    {
        error = "invalid SPIR-V shader module input";
        return false;
    }

    VkShaderModuleCreateInfo info{};
    info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    info.codeSize = size;
    info.pCode = code;
    if (dispatch.create(device, &info, nullptr, &m_module) != VK_SUCCESS)
    {
        m_module = VK_NULL_HANDLE;
        error = "vkCreateShaderModule failed";
        return false;
    }

    m_device = device;
    m_destroy = dispatch.destroy;
    error.clear();
    return true;
}

bool ShaderModule::initialize_bytes(VkDevice device, const ShaderModuleDispatch& dispatch,
    const void* bytes, size_t size, std::string& error)
{
    if (!bytes || size < 5 * sizeof(uint32_t) || size % sizeof(uint32_t))
    {
        destroy();
        error = "invalid SPIR-V shader module input";
        return false;
    }
    std::vector<uint32_t> words(size / sizeof(uint32_t));
    std::memcpy(words.data(), bytes, size);
    return initialize(device, dispatch, words.data(), size, error);
}

void ShaderModule::destroy()
{
    if (m_device && m_module && m_destroy)
        m_destroy(m_device, m_module, nullptr);
    m_device = VK_NULL_HANDLE;
    m_module = VK_NULL_HANDLE;
    m_destroy = nullptr;
}
}
