#pragma once

#include "DeferredPass.h"
#include "EngineTextureSource.h"
#include "ShaderMaterialLibrary.h"

#include <unordered_map>
#include <functional>

namespace xray::render::vulkan
{
class ScenePass;

// The same engine VFS DDS becomes a sampled material or UI descriptor. The
// caller waits for all in-flight frames before destroying the factory/passes.
class GameTextureFactory
{
public:
    ~GameTextureFactory() { destroy(); }
    GameTextureFactory() = default;
    GameTextureFactory(const GameTextureFactory&) = delete;
    GameTextureFactory& operator=(const GameTextureFactory&) = delete;

    bool initialize(VkDevice device, VkQueue queue, VkCommandPool pool,
        const VkPhysicalDeviceMemoryProperties& memory, bool bc_supported,
        const VkPhysicalDeviceFeatures& features, const VkPhysicalDeviceProperties& properties,
        float requested_anisotropy,
        const TextureUploadDispatch& dispatch, PFN_vkCreateSampler create_sampler,
        PFN_vkDestroySampler destroy_sampler, PFN_vkDeviceWaitIdle wait_idle,
        std::string& error);
    bool material(const std::string& texture_list, DeferredPass& pass,
        VkDescriptorSet& result, std::string& error);
    bool lightmapped_material(const std::string& diffuse, const std::string& lightmap,
        DeferredPass& pass, VkDescriptorSet& result, std::string& error);
    void release_lightmapped_material(VkDescriptorSet set, DeferredPass& pass);
    bool ui(const std::string& texture_name, ScenePass& pass,
        VkDescriptorSet& result, std::string& error, VkExtent2D* extent = nullptr);
    bool ui_pixels(const uint8_t* rgba, uint32_t width, uint32_t height,
        ScenePass& pass, VkDescriptorSet& result, std::string& error);
    // Weather textures are leased independently of material/UI descriptor sets.
    // Cubemaps retain their cube image view for sky sampling.
    bool environment(const std::string& name, VkImageView& view, std::string& error);
    void release_environment(VkImageView view);
    VkSampler sampler() const { return sampler_; }
    // Release only after the consumer has stopped recording the descriptor.
    // The last release waits for submitted frames before freeing GPU objects.
    void release_material(VkDescriptorSet set, DeferredPass &pass);
    void release_ui(VkDescriptorSet set, ScenePass &pass);
    bool retain_ui(VkDescriptorSet set, ScenePass &pass);
    bool finish_uploads();
    void retire_unused();
    void invalidate_unused();
    bool reload_assets(std::string& error, const std::function<void()>& before_rebind = {},
        const std::function<void()>& after_rebind = {});
    uint64_t resident_bytes() const;
    size_t resident_count() const { return assets_.size(); }
    bool load_blender_library(IReader& file, std::string& error)
    { return materials_.load(file, error); }
    size_t blender_count() const { return materials_.size(); }
    bool has_blender(const std::string& shader) const { return materials_.contains(shader); }
    bool surface_mode(const std::string& shader, const std::string& texture,
        SurfaceMode& mode, std::string& error, int* alpha_ref = nullptr,
        int* blend_mode = nullptr, bool particle_pipeline = false,
        bool screen_pipeline = false, bool level_pipeline = false,
        bool water_pipeline = false, uint32_t* material_flags = nullptr) const
    {
        if (materials_.size()) return materials_.resolve(shader, mode, error, alpha_ref, blend_mode,
            particle_pipeline, screen_pipeline, level_pipeline, water_pipeline, material_flags);
        // Standalone mock-GPU tests have no game archive; gameplay loads the
        // shaders.xr library at OnDeviceCreate before any level or OGF.
        mode = classify_surface_material(shader, texture);
        if (alpha_ref) *alpha_ref = 128;
        if (blend_mode) *blend_mode = -1;
        if (material_flags) *material_flags = 0;
        error.clear();
        return true;
    }
    void destroy();

private:
    struct Asset
    {
        UploadedTexture texture;
        VkDescriptorSet material_set{};
        VkDescriptorSet ui_set{};
        VkExtent2D extent{};
        uint64_t bytes{};
        size_t material_refs{};
        size_t lightmap_refs{};
        size_t ui_refs{};
        size_t environment_refs{};
        DeferredPass* material_pass{};
        ScenePass* ui_pass{};
    };
    struct LightmappedMaterial
    {
        VkDescriptorSet set{};
        Asset* diffuse{};
        Asset* lightmap{};
        DeferredPass* pass{};
        std::string diffuse_path, lightmap_path;
        size_t refs{};
    };
    bool load(const std::string& name, Asset*& asset, std::string& error);
    void evict_if_unused(const std::string& name);
    VkDevice device_{};
    VkQueue queue_{};
    VkCommandPool pool_{};
    VkPhysicalDeviceMemoryProperties memory_{};
    bool bc_supported_{};
    TextureUploadDispatch dispatch_{};
    PFN_vkDestroySampler destroy_sampler_{};
    PFN_vkDeviceWaitIdle wait_idle_{};
    VkSampler sampler_{};
    VkSampler ui_sampler_{};
    ImageStateTracker states_;
    std::vector<PendingTextureUpload> pending_;
    std::unordered_map<std::string, Asset> assets_;
    std::unordered_map<std::string, LightmappedMaterial> lightmapped_;
    uint64_t transient_id_{};
    ShaderMaterialLibrary materials_;
};
}
