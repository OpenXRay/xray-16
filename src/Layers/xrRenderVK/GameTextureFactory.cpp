#include "xrEngine/stdafx.h"
#include "GameTextureFactory.h"

#include "ScenePass.h"

#include <algorithm>
#include <cctype>

namespace xray::render::vulkan
{
namespace
{
bool retirement_idle(VkDevice device, PFN_vkDeviceWaitIdle wait)
{
    const VkResult result = wait(device);
    return result == VK_SUCCESS || result == VK_ERROR_DEVICE_LOST;
}
}
bool GameTextureFactory::initialize(VkDevice device, VkQueue queue, VkCommandPool pool,
    const VkPhysicalDeviceMemoryProperties& memory, bool bc_supported,
    const VkPhysicalDeviceFeatures& features, const VkPhysicalDeviceProperties& properties,
    float requested_anisotropy,
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
    if (features.samplerAnisotropy && properties.limits.maxSamplerAnisotropy > 1.f)
    {
        info.anisotropyEnable = VK_TRUE;
        info.maxAnisotropy = std::clamp(requested_anisotropy, 1.f,
            properties.limits.maxSamplerAnisotropy);
    }
    if (create_sampler(device_, &info, nullptr, &sampler_) != VK_SUCCESS)
    {
        error = "could not create game texture sampler";
        destroy();
        return false;
    }
    info.addressModeU = info.addressModeV = info.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    info.anisotropyEnable = VK_FALSE;
    info.maxAnisotropy = 1.f;
    if (create_sampler(device_, &info, nullptr, &ui_sampler_) != VK_SUCCESS)
    {
        error = "could not create UI texture sampler";
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
    // Particle and UI definitions commonly retain the source extension while
    // the mounted game texture was converted to DDS.
    if (normalized.size() >= 4)
    {
        std::string extension = normalized.substr(normalized.size() - 4);
        std::transform(extension.begin(), extension.end(), extension.begin(),
            [](unsigned char ch) { return static_cast<char>(std::tolower(ch)); });
        if (extension == ".dds" || extension == ".bmp" || extension == ".tga" || extension == ".png")
            normalized.resize(normalized.size() - 4);
    }
    string_path path;
    if (!FS.exist(path, "$level$", normalized.c_str(), ".dds") &&
        !FS.exist(path, "$game_textures$", normalized.c_str(), ".dds"))
    {
        error = "game DDS not found: " + normalized;
        return false;
    }
    // A new level may have a different $level$ texture with the same name.
    // Cache by the resolved VFS path, not the material's local texture name.
    const std::string key(path);
    auto existing = assets_.find(key);
    if (existing != assets_.end())
    {
        asset = &existing->second;
        error.clear();
        return true;
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
    if (!uploaded)
    {
        error = "texture '" + normalized + "': " + error;
        Msg("! [renderer-vulkan] texture.decode.failed %s", error.c_str());
        return false;
    }
    auto inserted = assets_.emplace(key, Asset{}).first;
    inserted->second.texture = texture;
    inserted->second.extent = {extent.width, extent.height};
    // Conservative RGBA-equivalent residency estimate for DDS mip chains.
    inserted->second.bytes = uint64_t(extent.width) * extent.height * 4;
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

bool GameTextureFactory::lightmapped_material(const std::string& diffuse_name,
    const std::string& lightmap_name, DeferredPass& pass, VkDescriptorSet& result, std::string& error)
{
    result = VK_NULL_HANDLE;
    const auto trim = [](std::string text)
    {
        const size_t first = text.find_first_not_of(" \t");
        if (first == std::string::npos) return std::string{};
        const size_t last = text.find_last_not_of(" \t");
        return text.substr(first, last - first + 1);
    };
    Asset* diffuse = nullptr;
    Asset* lightmap = nullptr;
    if (!load(trim(diffuse_name), diffuse, error) || !load(trim(lightmap_name), lightmap, error))
        return false;
    const auto diffuse_it = std::find_if(assets_.begin(), assets_.end(),
        [diffuse](const auto& entry) { return &entry.second == diffuse; });
    const auto lightmap_it = std::find_if(assets_.begin(), assets_.end(),
        [lightmap](const auto& entry) { return &entry.second == lightmap; });
    R_ASSERT(diffuse_it != assets_.end() && lightmap_it != assets_.end());
    std::string key = diffuse_it->first;
    key.push_back('\0');
    key += lightmap_it->first;
    auto found = lightmapped_.find(key);
    if (found == lightmapped_.end())
    {
        VkDescriptorSet descriptor{};
        if (!pass.lightmapped_material(diffuse->texture.view, lightmap->texture.view,
                sampler_, descriptor, error)) return false;
        LightmappedMaterial pair;
        pair.set = descriptor;
        pair.diffuse = diffuse;
        pair.lightmap = lightmap;
        pair.pass = &pass;
        pair.diffuse_path = diffuse_it->first;
        pair.lightmap_path = lightmap_it->first;
        found = lightmapped_.emplace(std::move(key), std::move(pair)).first;
        ++diffuse->lightmap_refs;
        if (diffuse != lightmap) ++lightmap->lightmap_refs;
    }
    R_ASSERT2(found->second.pass == &pass, "Vulkan lightmap belongs to another pass");
    ++found->second.refs;
    result = found->second.set;
    error.clear();
    return true;
}

void GameTextureFactory::release_lightmapped_material(VkDescriptorSet set, DeferredPass& pass)
{
    if (!set) return;
    for (auto it = lightmapped_.begin(); it != lightmapped_.end(); ++it)
    {
        auto& pair = it->second;
        if (pair.set != set) continue;
        R_ASSERT2(pair.pass == &pass && pair.refs, "unknown Vulkan lightmap lease");
        if (--pair.refs) return;
        R_ASSERT2(retirement_idle(device_, wait_idle_), "Vulkan lightmap retirement requires idle GPU frames");
        pass.release_gbuffer(pair.set);
        --pair.diffuse->lightmap_refs;
        if (pair.diffuse != pair.lightmap) --pair.lightmap->lightmap_refs;
        const std::string diffuse_path = pair.diffuse_path, lightmap_path = pair.lightmap_path;
        lightmapped_.erase(it);
        evict_if_unused(diffuse_path);
        if (lightmap_path != diffuse_path) evict_if_unused(lightmap_path);
        return;
    }
    R_ASSERT2(false, "unknown Vulkan lightmap descriptor");
}

bool GameTextureFactory::ui(const std::string& texture_name, ScenePass& pass,
    VkDescriptorSet& result, std::string& error, VkExtent2D* extent)
{
    result = VK_NULL_HANDLE;
    Asset* asset;
    if (!load(texture_name, asset, error)) return false;
    if (!asset->ui_set && !pass.create_ui_texture_set(asset->texture.view, ui_sampler_, asset->ui_set, error))
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

bool GameTextureFactory::ui_pixels(const uint8_t* rgba, uint32_t width, uint32_t height,
    ScenePass& pass, VkDescriptorSet& result, std::string& error)
{
    result = VK_NULL_HANDLE;
    if (!device_ || !rgba || !width || !height || width > 8192 || height > 8192)
    { error = "invalid dynamic UI texture"; return false; }
    DdsTexture pixels;
    pixels.format = VK_FORMAT_R8G8B8A8_UNORM;
    pixels.extent = {width, height, 1};
    pixels.mip_levels = 1;
    pixels.pixels.assign(rgba, rgba + size_t(width) * height * 4);
    VkBufferImageCopy copy{};
    copy.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    copy.imageSubresource.layerCount = 1;
    copy.imageExtent = {width, height, 1};
    pixels.copies.push_back(copy);
    Asset asset;
    if (!upload_texture(device_, queue_, pool_, memory_, dispatch_, pixels,
            asset.texture, pending_, states_, error)) return false;
    if (!pass.create_ui_texture_set(asset.texture.view, ui_sampler_, asset.ui_set, error))
    {
        R_ASSERT2(wait_for_uploads(device_, pool_, dispatch_, pending_),
            "dynamic UI upload did not finish");
        states_.forget_image(asset.texture.image);
        destroy_texture(device_, dispatch_, asset.texture);
        return false;
    }
    asset.extent = {width, height};
    asset.bytes = size_t(width) * height * 4;
    asset.ui_refs = 1;
    asset.ui_pass = &pass;
    result = asset.ui_set;
    assets_.emplace("#dynamic-ui-" + std::to_string(++transient_id_), std::move(asset));
    error.clear();
    return true;
}

void GameTextureFactory::evict_if_unused(const std::string& name)
{
    auto it = assets_.find(name);
    if (it == assets_.end() || it->second.material_refs || it->second.lightmap_refs || it->second.ui_refs ||
        it->second.environment_refs)
        return;
    R_ASSERT2(wait_for_uploads(device_, pool_, dispatch_, pending_),
        "Vulkan texture upload did not complete before release");
    states_.forget_image(it->second.texture.image);
    destroy_texture(device_, dispatch_, it->second.texture);
    assets_.erase(it);
}

bool GameTextureFactory::environment(const std::string& name, VkImageView& view,
    std::string& error)
{
    view = VK_NULL_HANDLE;
    Asset* asset = nullptr;
    if (!load(name, asset, error)) return false;
    ++asset->environment_refs;
    view = asset->texture.view;
    return true;
}

void GameTextureFactory::release_environment(VkImageView view)
{
    if (!view) return;
    for (auto& [name, asset] : assets_)
    {
        if (asset.texture.view != view) continue;
        R_ASSERT2(asset.environment_refs, "Vulkan environment texture is not leased");
        if (--asset.environment_refs) return;
        R_ASSERT2(retirement_idle(device_, wait_idle_),
            "Vulkan device did not become idle before environment texture release");
        evict_if_unused(name);
        return;
    }
    R_ASSERT2(false, "unknown Vulkan environment texture");
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
        R_ASSERT2(retirement_idle(device_, wait_idle_),
            "Vulkan device did not become idle before material release");
        pass.release_gbuffer(asset.material_set);
        asset.material_pass = nullptr;
        evict_if_unused(name);
        return;
    }
    R_ASSERT2(false, "unknown Vulkan material descriptor");
}

bool GameTextureFactory::retain_ui(VkDescriptorSet set, ScenePass &pass)
{
    if (!set)
        return false;
    for (auto &[name, asset] : assets_)
        if (asset.ui_set == set && asset.ui_refs && asset.ui_pass == &pass)
        {
            ++asset.ui_refs;
            return true;
        }
    return false;
}

void GameTextureFactory::release_ui(VkDescriptorSet set, ScenePass &pass)
{
    if (!set)
        return;
    for (auto& [name, asset] : assets_)
    {
        if (asset.ui_set != set) continue;
        R_ASSERT2(asset.ui_refs && asset.ui_pass == &pass,
            "Vulkan UI descriptor is not leased by this pass");
        if (--asset.ui_refs) return;
        R_ASSERT2(retirement_idle(device_, wait_idle_),
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
        R_ASSERT2(retirement_idle(device_, wait_idle_), "Vulkan device did not become idle before texture factory shutdown");
        wait_for_uploads(device_, pool_, dispatch_, pending_);
        for (auto& [name, pair] : lightmapped_)
            if (pair.set && pair.pass) pair.pass->release_gbuffer(pair.set);
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
        if (ui_sampler_ && destroy_sampler_) destroy_sampler_(device_, ui_sampler_, nullptr);
    }
    assets_.clear();
    lightmapped_.clear();
    transient_id_ = 0;
    pending_.clear();
    states_ = {};
    device_ = VK_NULL_HANDLE;
    queue_ = VK_NULL_HANDLE;
    pool_ = VK_NULL_HANDLE;
    sampler_ = VK_NULL_HANDLE;
    ui_sampler_ = VK_NULL_HANDLE;
    destroy_sampler_ = nullptr;
    wait_idle_ = nullptr;
    dispatch_ = {};
}

bool GameTextureFactory::finish_uploads()
{
    return !device_ || wait_for_uploads(device_, pool_, dispatch_, pending_);
}

void GameTextureFactory::retire_unused()
{
    if (!device_) return;
    R_ASSERT2(retirement_idle(device_, wait_idle_), "Vulkan texture retirement requires idle frames");
    for (auto it = assets_.begin(); it != assets_.end();)
    {
        if (it->second.material_refs || it->second.lightmap_refs || it->second.ui_refs || it->second.environment_refs)
        { ++it; continue; }
        auto name = it->first;
        ++it;
        evict_if_unused(name);
    }
}

void GameTextureFactory::invalidate_unused()
{
    // Active descriptors retain their image views until consumers release them.
    // Newly requested assets resolve their VFS path again after retirement.
    retire_unused();
}

bool GameTextureFactory::reload_assets(std::string& error,
    const std::function<void()>& before_rebind, const std::function<void()>& after_rebind)
{
    if (!device_) { error = "Vulkan asset reload needs a device"; return false; }
    if (wait_idle_(device_) != VK_SUCCESS || !finish_uploads())
    { error = "Vulkan asset reload needs idle frames and completed uploads"; return false; }
    struct Staged
    {
        Asset* asset{};
        UploadedTexture texture;
        VkExtent3D extent{};
    };
    std::vector<Staged> staged;
    auto discard = [&]
    {
        if (!finish_uploads())
            R_ASSERT2(wait_idle_(device_) == VK_SUCCESS, "Vulkan reload staging needs an idle device");
        for (auto& item : staged)
        {
            states_.forget_image(item.texture.image);
            destroy_texture(device_, dispatch_, item.texture);
        }
    };
    for (auto& [path, asset] : assets_)
    {
        if (path[0] == '#') continue;
        IReader* source = FS.r_open(path.c_str());
        if (!source)
        { error = "cannot reopen texture during asset reload: " + path; discard(); return false; }
        UploadedTexture replacement;
        VkExtent3D extent{};
        const bool loaded = upload_engine_texture(device_, queue_, pool_, memory_, dispatch_,
            source->pointer(), source->length(), bc_supported_, replacement, pending_, states_,
            error, &extent);
        FS.r_close(source);
        if (!loaded)
        {
            error = "texture reload " + path + ": " + error;
            discard();
            return false;
        }
        staged.push_back({&asset, replacement, extent});
    }
    if (!finish_uploads())
    { error = "texture reload uploads failed"; discard(); return false; }
    // Keep environment entries alive while their old view leases are dropped.
    // The callback clears the weather descriptor before any old view is freed.
    std::vector<Asset*> environment_assets;
    if (before_rebind)
    {
        for (auto& [path, asset] : assets_)
            if (asset.environment_refs)
            { ++asset.environment_refs; environment_assets.push_back(&asset); }
        before_rebind();
    }
    for (auto& item : staged)
    {
        Asset& asset = *item.asset;
        if (asset.material_set && asset.material_pass)
            asset.material_pass->update_material(asset.material_set, item.texture.view, sampler_);
        if (asset.ui_set && asset.ui_pass)
            asset.ui_pass->update_ui_texture_set(asset.ui_set, item.texture.view, ui_sampler_);
        states_.forget_image(asset.texture.image);
        destroy_texture(device_, dispatch_, asset.texture);
        asset.texture = item.texture;
        asset.extent = {item.extent.width, item.extent.height};
        asset.bytes = uint64_t(item.extent.width) * item.extent.height * 4;
    }
    for (auto& [name, pair] : lightmapped_)
        pair.pass->update_lightmapped_material(pair.set, pair.diffuse->texture.view,
            pair.lightmap->texture.view, sampler_);
    if (after_rebind) after_rebind();
    for (auto* asset : environment_assets)
    {
        R_ASSERT2(asset->environment_refs, "Vulkan weather reload lost its staging lease");
        --asset->environment_refs;
    }
    retire_unused();
    error.clear();
    return true;
}

uint64_t GameTextureFactory::resident_bytes() const
{
    uint64_t bytes = 0;
    for (const auto& [name, asset] : assets_) bytes += asset.bytes;
    return bytes;
}
}
