#pragma once

#include "xrEngine/Render.h"
#include "GpuLevel.h"

#include <memory>

namespace xray::render::vulkan
{
class VulkanGameDevice;
// Shared IRender level contract for the Vulkan gameplay renderer. Its caller
// must have created a device, deferred pass and texture factory first. Other
// IRender operations remain abstract until their Vulkan implementations exist.
class VulkanLevelRender : public IRender
{
public:
    ~VulkanLevelRender() override;
    void bind_level_device(VulkanGameDevice& resources);
    void bind_level_device(VkDevice device, VkQueue queue, VkCommandPool pool,
        const VkPhysicalDeviceMemoryProperties& memory, const BufferUploadDispatch& upload,
        GameTextureFactory& textures, DeferredPass& pass, PFN_vkDeviceWaitIdle wait_idle);
    void level_Load(IReader* reader) override;
    void level_Unload() override;
    void reset_begin() override;
    void reset_end() override;
    IRenderVisual* getVisual(int index) override;
    void Create(SDL_Window* window, u32& width, u32& height,
        float& half_width, float& half_height) override;
    void Destroy() override;
    void Reset(SDL_Window* window, u32& width, u32& height,
        float& half_width, float& half_height) override;
    void SetupStates() override;
    void OnDeviceCreate(pcstr shader_archive) override;
    void OnDeviceDestroy(bool keep_textures) override;
    void Begin() override;
    void Calculate() override;
    void Render() override;
    void RenderMenu() override;
    void Clear() override;
    void End() override;
    void ClearTarget() override;
    void add_Visual(u32 context_id, IRenderable* root, IRenderVisual* visual,
        Fmatrix& world) override;
    DeviceState GetDeviceState() override;
    void OnAppLifecycleChanged(bool active) override;

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
    VulkanGameDevice* game_device_{};
    SDL_Window* window_{};
    VkExtent2D requested_drawable_{};
    std::unique_ptr<VulkanGameDevice> owned_game_device_;
    bool device_resources_ready_{};
    bool reset_in_progress_{};
    bool reset_pending_{};
    bool app_suspended_{};
    bool recreate_surface_pending_{};
    bool frame_active_{};
    bool world_calculated_{};
    bool world_rendered_{};
    bool clear_target_pending_{};
    bool frame_clear_target_{};
};
}
