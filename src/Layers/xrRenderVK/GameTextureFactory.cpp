#include "xrEngine/stdafx.h"
#include "GameTextureFactory.h"

#include "ScenePass.h"

#include <algorithm>
#include <cctype>

namespace xray::render::vulkan
{
bool GameTextureFactory::initialize(VkDevice device, VkQueue queue, VkCommandPool pool,
    const VkPhysicalDeviceMemoryProperties& memory, bool bc_supported,
    const TextureUploadDispatch& dispatch, PFN_vkCreateSampler create_sampler,
    PFN_vkDestroySampler destroy_sampler, PFN_vkDeviceWaitIdle wait_idle,
    std::string& error)
{
    destroy();
    if (!device || !queue || !pool || !create_sampler || !destroy_sampler || !wait_idle ||
        !dispatch.create_image || !dispatch.create_buffer || !dispatch.queue_submit ||
        !dispatch.destroy_image || !dispatch.destroy_image_view || !dispatch.free_memory ||
        !dispatch.wait_for_fences || !dispatch.free_command_buffers)
    {
        error = "game texture factory needs a Vulkan device and upload procedures";
        return false;
    }
    device_ = device;
    queue_ = queue;
    pool_ = pool;
    memory_ = memory;
    bc_supported_ = bc_supported;
    dispatch_ = dispatch;
    destroy_sampler_ = destroy_sampler;
    wait_idle_ = wait_idle;
    VkSamplerCreateInfo info{VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO};
    info.magFilter = info.minFilter = VK_FILTER_LINEAR;
    info.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    info.addressModeU = info.addressModeV = info.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    info.maxLod = VK_LOD_CLAMP_NONE;
    if (create_sampler(device_, &info, nullptr, &sampler_) != VK_SUCCESS)
    {
        error = "could not create game texture sampler";
        destroy();
        return false;
    }
    error.clear();
    return true;
}

bool GameTextureFactory::load(const std::string& name, Asset*& asset, std::string& error)
{
    asset = nullptr;
    if (!device_ || !sampler_ || name.empty())
    {
        error = "game texture factory or name is invalid";
        return false;
    }
    std::string normalized = name;
    if (normalized.size() >= 4 && normalized.compare(normalized.size() - 4, 4, ".dds") == 0)
        normalized.resize(normalized.size() - 4);
    auto existing = assets_.find(normalized);
    if (existing != assets_.end())
    {
        asset = &existing->second;
        error.clear();
        return true;
    }
    string_path path;
    if (!FS.exist(path, "$level$", normalized.c_str(), ".dds") &&
        !FS.exist(path, "$game_textures$", normalized.c_str(), ".dds"))
    {
        error = "game DDS not found: " + normalized;
        return false;
    }
    IReader* reader = FS.r_open(path);
    if (!reader)
    {
        error = "game DDS cannot be opened: " + normalized;
        return false;
    }
    UploadedTexture texture;
    VkExtent3D extent{};
    const bool uploaded = upload_engine_texture(device_, queue_, pool_, memory_, dispatch_,
        reader->pointer(), reader->length(), bc_supported_, texture, pending_, states_, error, &extent);
    FS.r_close(reader);
    if (!uploaded) return false;
    auto inserted = assets_.emplace(normalized, Asset{}).first;
    inserted->second.texture = texture;
    inserted->second.extent = {extent.width, extent.height};
    asset = &inserted->second;
    return true;
}

bool GameTextureFactory::material(const std::string& texture_list, DeferredPass& pass,
    VkDescriptorSet& result, std::string& error)
{
    result = VK_NULL_HANDLE;
    const size_t comma = texture_list.find(',');
    std::string diffuse = texture_list.substr(0, comma);
    while (!diffuse.empty() && std::isspace(static_cast<unsigned char>(diffuse.back())))
        diffuse.pop_back();
    const size_t first = diffuse.find_first_not_of(" \t");
    if (first != std::string::npos) diffuse.erase(0, first);
    Asset* asset;
    if (!load(diffuse, asset, error)) return false;
    if (!asset->material_set && !pass.material(asset->texture.view, sampler_, asset->material_set, error))
    {
        const auto it = std::find_if(assets_.begin(), assets_.end(),
            [asset](const auto& entry) { return &entry.second == asset; });
        if (it != assets_.end()) evict_if_unused(it->first);
        return false;
    }
    R_ASSERT2(!asset->material_pass || asset->material_pass == &pass,
        "Vulkan material descriptor belongs to a different pass");
    asset->material_pass = &pass;
    ++asset->material_refs;
    result = asset->material_set;
    error.clear();
    return true;
}

bool GameTextureFactory::ui(const std::string& texture_name, ScenePass& pass,
    VkDescriptorSet& result, std::string& error, VkExtent2D* extent)
{
    result = VK_NULL_HANDLE;
    Asset* asset;
    if (!load(texture_name, asset, error)) return false;
    if (!asset->ui_set && !pass.create_ui_texture_set(asset->texture.view, sampler_, asset->ui_set, error))
    {
        const auto it = std::find_if(assets_.begin(), assets_.end(),
            [asset](const auto& entry) { return &entry.second == asset; });
        if (it != assets_.end()) evict_if_unused(it->first);
        return false;
    }
    R_ASSERT2(!asset->ui_pass || asset->ui_pass == &pass,
        "Vulkan UI descriptor belongs to a different pass");
    asset->ui_pass = &pass;
    ++asset->ui_refs;
    result = asset->ui_set;
    if (extent) *extent = asset->extent;
    error.clear();
    return true;
}

void GameTextureFactory::evict_if_unused(const std::string& name)
{
    auto it = assets_.find(name);
    if (it == assets_.end() || it->second.material_refs || it->second.ui_refs)
        return;
    R_ASSERT2(wait_for_uploads(device_, pool_, dispatch_, pending_),
        "Vulkan texture upload did not complete before release");
    states_.forget_image(it->second.texture.image);
    destroy_texture(device_, dispatch_, it->second.texture);
    assets_.erase(it);
}

void GameTextureFactory::release_material(VkDescriptorSet set, DeferredPass& pass)
{
    if (!set) return;
    for (auto& [name, asset] : assets_)
    {
        if (asset.material_set != set) continue;
        R_ASSERT2(asset.material_refs && asset.material_pass == &pass,
            "Vulkan material descriptor is not leased by this pass");
        if (--asset.material_refs) return;
        R_ASSERT2(wait_idle_(device_) == VK_SUCCESS,
            "Vulkan device did not become idle before material release");
        pass.release_gbuffer(asset.material_set);
        asset.material_pass = nullptr;
        evict_if_unused(name);
        return;
    }
    R_ASSERT2(false, "unknown Vulkan material descriptor");
}

void GameTextureFactory::release_ui(VkDescriptorSet set, ScenePass& pass)
{
    if (!set) return;
    for (auto& [name, asset] : assets_)
    {
        if (asset.ui_set != set) continue;
        R_ASSERT2(asset.ui_refs && asset.ui_pass == &pass,
            "Vulkan UI descriptor is not leased by this pass");
        if (--asset.ui_refs) return;
        R_ASSERT2(wait_idle_(device_) == VK_SUCCESS,
            "Vulkan device did not become idle before UI texture release");
        pass.release_ui_texture_set(asset.ui_set);
        asset.ui_pass = nullptr;
        evict_if_unused(name);
        return;
    }
    R_ASSERT2(false, "unknown Vulkan UI descriptor");
}

void GameTextureFactory::destroy()
{
    if (device_)
    {
        R_ASSERT2(wait_idle_(device_) == VK_SUCCESS,
            "Vulkan device did not become idle before texture factory shutdown");
        wait_for_uploads(device_, pool_, dispatch_, pending_);
        for (auto& [name, asset] : assets_)
        {
            if (asset.material_set && asset.material_pass)
                asset.material_pass->release_gbuffer(asset.material_set);
            if (asset.ui_set && asset.ui_pass)
                asset.ui_pass->release_ui_texture_set(asset.ui_set);
            states_.forget_image(asset.texture.image);
            destroy_texture(device_, dispatch_, asset.texture);
        }
        if (sampler_ && destroy_sampler_) destroy_sampler_(device_, sampler_, nullptr);
    }
    assets_.clear();
    pending_.clear();
    states_ = {};
    device_ = VK_NULL_HANDLE;
    queue_ = VK_NULL_HANDLE;
    pool_ = VK_NULL_HANDLE;
    sampler_ = VK_NULL_HANDLE;
    destroy_sampler_ = nullptr;
    wait_idle_ = nullptr;
    dispatch_ = {};
}
}
