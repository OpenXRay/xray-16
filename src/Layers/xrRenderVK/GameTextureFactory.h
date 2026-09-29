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
        PFN_vkDestroySampler destroy_sampler, std::string& error);
    bool material(const std::string& texture_list, DeferredPass& pass,
        VkDescriptorSet& result, std::string& error);
    bool ui(const std::string& texture_name, ScenePass& pass,
        VkDescriptorSet& result, std::string& error);
    void destroy();

private:
    struct Asset
    {
        UploadedTexture texture;
        VkDescriptorSet material_set{};
        VkDescriptorSet ui_set{};
    };
    bool load(const std::string& name, Asset*& asset, std::string& error);
    VkDevice device_{};
    VkQueue queue_{};
    VkCommandPool pool_{};
    VkPhysicalDeviceMemoryProperties memory_{};
    bool bc_supported_{};
    TextureUploadDispatch dispatch_{};
    PFN_vkDestroySampler destroy_sampler_{};
    VkSampler sampler_{};
    ImageStateTracker states_;
    std::vector<PendingTextureUpload> pending_;
    std::unordered_map<std::string, Asset> assets_;
};
}
