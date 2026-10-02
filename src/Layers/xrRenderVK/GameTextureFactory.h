#pragma once

#include "DeferredPass.h"
#include "EngineTextureSource.h"

#include <unordered_map>

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
        const TextureUploadDispatch& dispatch, PFN_vkCreateSampler create_sampler,
        PFN_vkDestroySampler destroy_sampler, PFN_vkDeviceWaitIdle wait_idle,
        std::string& error);
    bool material(const std::string& texture_list, DeferredPass& pass,
        VkDescriptorSet& result, std::string& error);
    bool ui(const std::string& texture_name, ScenePass& pass,
        VkDescriptorSet& result, std::string& error, VkExtent2D* extent = nullptr);
    bool ui_pixels(const uint8_t* rgba, uint32_t width, uint32_t height,
        ScenePass& pass, VkDescriptorSet& result, std::string& error);
    // Release only after the consumer has stopped recording the descriptor.
    // The last release waits for submitted frames before freeing GPU objects.
    void release_material(VkDescriptorSet set, DeferredPass& pass);
    void release_ui(VkDescriptorSet set, ScenePass& pass);
    void destroy();

private:
    struct Asset
    {
        UploadedTexture texture;
        VkDescriptorSet material_set{};
        VkDescriptorSet ui_set{};
        VkExtent2D extent{};
        size_t material_refs{};
        size_t ui_refs{};
        DeferredPass* material_pass{};
        ScenePass* ui_pass{};
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
    ImageStateTracker states_;
    std::vector<PendingTextureUpload> pending_;
    std::unordered_map<std::string, Asset> assets_;
    uint64_t transient_id_{};
};
}
