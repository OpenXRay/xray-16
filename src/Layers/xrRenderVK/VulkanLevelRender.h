#pragma once

#include "xrEngine/Render.h"
#include "GpuLevel.h"
#include "VulkanCameraState.h"
#include "VulkanDeviceResourceState.h"
#include "VulkanFramePhaseState.h"
#include "VulkanGameLighting.h"
#include "VulkanRenderContextState.h"

#include <memory>
#include <unordered_map>
#include <vector>

namespace xray::render::vulkan
{
class VulkanGameDevice;
class VulkanFontRender;
class VulkanUIShader;
class VulkanModelVisual;
// Shared IRender level contract for the Vulkan gameplay renderer. Its caller
// must have created a device, deferred pass and texture factory first. Other
// IRender operations remain abstract until their Vulkan implementations exist.
class VulkanLevelRender : public IRender
{
public:
    ~VulkanLevelRender() override;
    void bind_level_device(VulkanGameDevice& resources);
    std::unique_ptr<VulkanUIShader> create_ui_shader();
    std::unique_ptr<VulkanFontRender> create_font_render();
    void bind_level_device(VkDevice device, VkQueue queue, VkCommandPool pool,
        const VkPhysicalDeviceMemoryProperties& memory, const BufferUploadDispatch& upload,
        GameTextureFactory& textures, DeferredPass& pass, PFN_vkDeviceWaitIdle wait_idle);
    void level_Load(IReader* reader) override;
    void level_Unload() override;
    void reset_begin() override;
    void reset_end() override;
    IRenderVisual* getVisual(int index) override;
    IRenderVisual* model_Create(pcstr name, IReader* data = nullptr) override;
    IRenderVisual* model_CreateChild(pcstr name, IReader* data) override;
    void model_Delete(IRenderVisual*& visual, bool discard = false) override;
    void models_Clear(bool complete) override;
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
    void OnCameraUpdated() override;
    void SetCacheXform(Fmatrix& view, Fmatrix& project) override;
    RenderContext GetCurrentContext() const override;
    void MakeContextCurrent(RenderContext context) override;
    void add_Visual(u32 context_id, IRenderable* root, IRenderVisual* visual,
        Fmatrix& world) override;
    IRender_ObjectSpecific* ros_create(IRenderable* parent) override;
    void ros_destroy(IRender_ObjectSpecific*& object) override;
    IRender_Light* light_create() override;
    void light_destroy(IRender_Light* light) override;
    IRender_Glow* glow_create() override;
    void glow_destroy(IRender_Glow* glow) override;
    DeviceState GetDeviceState() override;
    void OnAppLifecycleChanged(bool active) override;

protected:
    GpuLevel& gpu_level() { return level_; }
    const GpuLevel& gpu_level() const { return level_; }

private:
    Fmatrix current_view_projection() const;
    GpuLevel level_;
    std::unordered_map<IRenderVisual*, std::unique_ptr<VulkanModelVisual>> models_;
    VulkanCameraState camera_state_;
    VulkanRenderContextState context_state_;
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
    std::vector<VulkanLight*> lights_;
    std::vector<VulkanGlow*> glows_;
    std::vector<VulkanObjectSpecific*> object_specifics_;
    VulkanDeviceResourceState device_resource_state_;
    bool reset_in_progress_{};
    bool reset_pending_{};
    bool app_suspended_{};
    bool recreate_surface_pending_{};
    VulkanFramePhaseState frame_phase_;
    bool clear_target_pending_{};
    bool frame_clear_target_{};
};
}
