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
    PFN_vkDestroySampler destroy_sampler, std::string& error)
{
    destroy();
    if (!device || !queue || !pool || !create_sampler || !destroy_sampler ||
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
    const bool uploaded = upload_engine_texture(device_, queue_, pool_, memory_, dispatch_,
        reader->pointer(), reader->length(), bc_supported_, texture, pending_, states_, error);
    FS.r_close(reader);
    if (!uploaded) return false;
    auto inserted = assets_.emplace(normalized, Asset{}).first;
    inserted->second.texture = texture;
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
        return false;
    result = asset->material_set;
    error.clear();
    return true;
}

bool GameTextureFactory::ui(const std::string& texture_name, ScenePass& pass,
    VkDescriptorSet& result, std::string& error)
{
    result = VK_NULL_HANDLE;
    Asset* asset;
    if (!load(texture_name, asset, error)) return false;
    if (!asset->ui_set && !pass.create_ui_texture_set(asset->texture.view, sampler_, asset->ui_set, error))
        return false;
    result = asset->ui_set;
    error.clear();
    return true;
}

void GameTextureFactory::destroy()
{
    if (device_)
    {
        wait_for_uploads(device_, pool_, dispatch_, pending_);
        for (auto& [name, asset] : assets_)
            destroy_texture(device_, dispatch_, asset.texture);
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
    dispatch_ = {};
}
}
