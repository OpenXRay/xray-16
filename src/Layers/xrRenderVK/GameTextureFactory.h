#pragma once

#include "DeferredPass.h"
#include "EngineTextureSource.h"
#include "ShaderMaterialLibrary.h"
#include "ModelTessellation.h"

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
    struct DetailMetadata
    {
        std::string texture;
        float scale{1.f};
        bool diffuse{}, normal{};
        std::string bump_texture, external_normal_texture;
        uint32_t bump_mode{1}, material{1};
        float bump_height{.05f}, material_weight{};
    };
    struct MetadataExtras
    {
        const uint8_t* bump{};
        size_t bump_size{};
        const uint8_t* material{};
        size_t material_size{};
        const uint8_t* external_normal{};
        size_t external_normal_size{};
    };
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
        VkDescriptorSet& result, std::string& error, bool clamp = false);
    bool lightmapped_material(const std::string& diffuse, const std::string& lightmap,
        DeferredPass& pass, VkDescriptorSet& result, std::string& error);
    void release_lightmapped_material(VkDescriptorSet set, DeferredPass& pass);
    bool terrain_material(const std::string& diffuse, const std::string& lightmap,
        const TerrainMaterialTextures& terrain, DeferredPass& pass,
        VkDescriptorSet& result, std::string& error);
    bool detail_material(const std::string& diffuse, const std::string& lightmap,
        const std::string& detail, DeferredPass& pass,
        VkDescriptorSet& result, std::string& error);
    bool environment_material(const std::string& diffuse, const std::string& lightmap,
        const std::string& environment, bool cube, DeferredPass& pass,
        VkDescriptorSet& result, std::string& error);
    bool surface_material(const std::array<std::string, 20>& names,
        const SurfaceParameters& parameters, DeferredPass& pass,
        VkDescriptorSet& result, std::string& error, bool cube_environment = false,
        bool clamp = false);
    void release_terrain_material(VkDescriptorSet set, DeferredPass& pass);
    float terrain_detail_scale(const std::string& diffuse) const;
    bool texture_detail_metadata(const std::string& diffuse, DetailMetadata& detail,
        std::string& error) const;
    bool height_field(const std::string& bump_aux, const HeightField*& result,
        std::string& error);
    static bool decode_detail_metadata(const uint8_t* params, size_t params_size,
        const uint8_t* extension, size_t extension_size,
        DetailMetadata& detail, std::string& error)
    {
        return decode_detail_metadata(params, params_size, extension, extension_size,
            detail, error, MetadataExtras{});
    }
    static bool decode_detail_metadata(const uint8_t* params, size_t params_size,
        const uint8_t* extension, size_t extension_size,
        DetailMetadata& detail, std::string& error, const MetadataExtras& extras);
    bool ui(const std::string& texture_name, ScenePass& pass,
        VkDescriptorSet& result, std::string& error, VkExtent2D* extent = nullptr, bool clamp = true);
    bool ui_pixels(const uint8_t* rgba, uint32_t width, uint32_t height,
        ScenePass& pass, VkDescriptorSet& result, std::string& error, bool clamp = true);
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
        bool water_pipeline = false, uint32_t* material_flags = nullptr,
        TerrainMaterialTextures* terrain = nullptr,
        MaterialDefinition* definition = nullptr, bool model_pipeline = false) const
    {
        if (materials_.size()) return materials_.resolve(shader, mode, error, alpha_ref, blend_mode,
            particle_pipeline, screen_pipeline, level_pipeline, water_pipeline, material_flags, terrain,
            definition, model_pipeline);
        mode = SurfaceMode::Unresolved;
        error = "shaders.xr material library is not loaded for shader='" + shader +
            "' texture='" + texture + "'";
        return false;
    }
    void destroy();

private:
    std::unordered_map<std::string, HeightField> height_fields_;
    struct Asset
    {
        UploadedTexture texture;
        VkDescriptorSet material_set{};
        VkDescriptorSet clamp_material_set{};
        VkDescriptorSet ui_set{};
        VkDescriptorSet wrap_ui_set{};
        VkExtent2D extent{};
        bool cube{};
        uint64_t bytes{};
        size_t material_refs{};
        size_t clamp_material_refs{};
        size_t lightmap_refs{};
        size_t ui_refs{};
        size_t wrap_ui_refs{};
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
    struct TerrainMaterial
    {
        VkDescriptorSet set{};
        std::array<Asset*, 20> assets{};
        std::array<std::string, 20> paths{};
        size_t count{7};
        bool clamp{};
        DeferredPass* pass{};
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
    std::unordered_map<std::string, TerrainMaterial> terrain_;
    uint64_t transient_id_{};
    ShaderMaterialLibrary materials_;
};
}
