#pragma once

#include "xrEngine/Render.h"
#include "GpuLevel.h"

namespace xray::render::vulkan
{
class VulkanGameDevice;
// Shared IRender level contract for the Vulkan gameplay renderer. Its caller
// must have created a device, deferred pass and texture factory first. Other
// IRender operations remain abstract until their Vulkan implementations exist.
class VulkanLevelRender : public IRender
{
public:
    void bind_level_device(VulkanGameDevice& resources);
    void bind_level_device(VkDevice device, VkQueue queue, VkCommandPool pool,
        const VkPhysicalDeviceMemoryProperties& memory, const BufferUploadDispatch& upload,
        GameTextureFactory& textures, DeferredPass& pass, PFN_vkDeviceWaitIdle wait_idle);
    void level_Load(IReader* reader) override;
    void level_Unload() override;
    IRenderVisual* getVisual(int index) override;

protected:
    GpuLevel& gpu_level() { return level_; }
    const GpuLevel& gpu_level() const { return level_; }

private:
    GpuLevel level_;
    VkDevice device_{};
    VkQueue queue_{};
    VkCommandPool pool_{};
    VkPhysicalDeviceMemoryProperties memory_{};
    BufferUploadDispatch upload_{};
    GameTextureFactory* textures_{};
    DeferredPass* pass_{};
    PFN_vkDeviceWaitIdle wait_idle_{};
};
}
