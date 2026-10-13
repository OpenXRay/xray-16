#include "xrEngine/stdafx.h"
#include "GameTextureFactory.h"

#include "ScenePass.h"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstring>

namespace xray::render::vulkan
{
namespace
{
bool retirement_idle(VkDevice device, PFN_vkDeviceWaitIdle wait)
{
    const VkResult result = wait(device);
    return result == VK_SUCCESS || result == VK_ERROR_DEVICE_LOST;
}
std::string trim_texture_name(const std::string& name)
{
    const size_t first = name.find_first_not_of(" \t");
    if (first == std::string::npos) return {};
    const size_t last = name.find_last_not_of(" \t");
    return name.substr(first, last - first + 1);
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
    bool cube = false;
    const bool uploaded = upload_engine_texture(device_, queue_, pool_, memory_, dispatch_,
        reader->pointer(), reader->length(), bc_supported_, texture, pending_, states_, error, &extent, &cube);
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
    inserted->second.cube = cube;
    // Conservative RGBA-equivalent residency estimate for DDS mip chains.
    inserted->second.bytes = uint64_t(extent.width) * extent.height * 4;
    asset = &inserted->second;
    return true;
}

bool GameTextureFactory::material(const std::string& texture_list, DeferredPass& pass,
    VkDescriptorSet& result, std::string& error, bool clamp)
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
    if (asset->cube)
    {
        error = "base texture slot 0 requires 2D DDS: " + diffuse;
        return false;
    }
    VkDescriptorSet& set = clamp ? asset->clamp_material_set : asset->material_set;
    if (!set && !pass.material(asset->texture.view, clamp ? ui_sampler_ : sampler_, set, error))
    {
        const auto it = std::find_if(assets_.begin(), assets_.end(),
            [asset](const auto& entry) { return &entry.second == asset; });
        if (it != assets_.end()) evict_if_unused(it->first);
        return false;
    }
    R_ASSERT2(!asset->material_pass || asset->material_pass == &pass,
        "Vulkan material descriptor belongs to a different pass");
    asset->material_pass = &pass;
    ++(clamp ? asset->clamp_material_refs : asset->material_refs);
    result = set;
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
    if (diffuse->cube || lightmap->cube)
    {
        error = std::string("lightmapped texture slot ") + (diffuse->cube ? "0 (base): " + diffuse_name :
            "1 (lightmap): " + lightmap_name) + " requires 2D DDS";
        return false;
    }
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

bool GameTextureFactory::terrain_material(const std::string& diffuse_name,
    const std::string& lightmap_name, const TerrainMaterialTextures& channels,
    DeferredPass& pass, VkDescriptorSet& result, std::string& error)
{
    result = VK_NULL_HANDLE;
    if (!channels.enabled) { error = "terrain channels are missing"; return false; }
    TerrainMaterial item;
    const std::string diffuse = trim_texture_name(diffuse_name);
    const std::array<std::string, 7> names{diffuse, trim_texture_name(lightmap_name),
        diffuse + "_mask", channels.detail[0], channels.detail[1],
        channels.detail[2], channels.detail[3]};
    std::string key;
    for (size_t i = 0; i < names.size(); ++i)
    {
        if (!load(names[i], item.assets[i], error)) return false;
        if (item.assets[i]->cube)
        {
            constexpr const char* roles[]{"base", "lightmap", "mask", "R2-R", "R2-G", "R2-B", "R2-A"};
            error = "terrain texture slot " + std::to_string(i) + " (" + roles[i] +
                ") requires 2D DDS: " + names[i];
            return false;
        }
        const auto it = std::find_if(assets_.begin(), assets_.end(),
            [asset = item.assets[i]](const auto& entry) { return &entry.second == asset; });
        R_ASSERT(it != assets_.end());
        item.paths[i] = it->first;
        key += it->first;
        key.push_back('\0');
    }
    auto found = terrain_.find(key);
    if (found == terrain_.end())
    {
        std::array<VkImageView, 7> views{};
        for (size_t i = 0; i < views.size(); ++i) views[i] = item.assets[i]->texture.view;
        if (!pass.terrain_material(views, sampler_, item.set, error)) return false;
        item.pass = &pass;
        for (size_t i = 0; i < item.assets.size(); ++i)
            if (std::find(item.assets.begin(), item.assets.begin() + i, item.assets[i]) == item.assets.begin() + i)
                ++item.assets[i]->lightmap_refs;
        found = terrain_.emplace(std::move(key), std::move(item)).first;
    }
    R_ASSERT2(found->second.pass == &pass, "Vulkan terrain belongs to another pass");
    ++found->second.refs;
    result = found->second.set;
    error.clear();
    return true;
}

bool GameTextureFactory::detail_material(const std::string& diffuse_name,
    const std::string& lightmap_name, const std::string& detail_name,
    DeferredPass& pass, VkDescriptorSet& result, std::string& error)
{
    result = VK_NULL_HANDLE;
    TerrainMaterial item;
    const std::array<std::string, 7> names{
        trim_texture_name(diffuse_name), trim_texture_name(lightmap_name),
        trim_texture_name(detail_name), trim_texture_name(detail_name),
        trim_texture_name(detail_name), trim_texture_name(detail_name),
        trim_texture_name(detail_name)};
    std::string key = "single-detail";
    key.push_back('\0');
    for (size_t i = 0; i < names.size(); ++i)
    {
        if (!load(names[i], item.assets[i], error)) return false;
        if (item.assets[i]->cube)
        {
            error = "single detail texture slot " + std::to_string(i) +
                (i == 0 ? " (base)" : i == 1 ? " (lightmap)" : " (detail)") +
                " requires 2D DDS: " + names[i];
            return false;
        }
        const auto it = std::find_if(assets_.begin(), assets_.end(),
            [asset = item.assets[i]](const auto& entry) { return &entry.second == asset; });
        R_ASSERT(it != assets_.end());
        item.paths[i] = it->first;
        key += it->first;
        key.push_back('\0');
    }
    auto found = terrain_.find(key);
    if (found == terrain_.end())
    {
        std::array<VkImageView, 7> views{};
        for (size_t i = 0; i < views.size(); ++i) views[i] = item.assets[i]->texture.view;
        if (!pass.terrain_material(views, sampler_, item.set, error)) return false;
        item.pass = &pass;
        for (size_t i = 0; i < item.assets.size(); ++i)
            if (std::find(item.assets.begin(), item.assets.begin() + i, item.assets[i]) == item.assets.begin() + i)
                ++item.assets[i]->lightmap_refs;
        found = terrain_.emplace(std::move(key), std::move(item)).first;
    }
    R_ASSERT2(found->second.pass == &pass, "Vulkan detail belongs to another pass");
    ++found->second.refs;
    result = found->second.set;
    error.clear();
    return true;
}

bool GameTextureFactory::environment_material(const std::string& diffuse_name,
    const std::string& lightmap_name, const std::string& environment_name, bool cube,
    DeferredPass& pass, VkDescriptorSet& result, std::string& error)
{
    result = VK_NULL_HANDLE;
    TerrainMaterial item;
    const std::string diffuse = trim_texture_name(diffuse_name);
    const std::string lightmap = lightmap_name.empty() ? diffuse : trim_texture_name(lightmap_name);
    const std::string environment = trim_texture_name(environment_name);
    const std::array<std::string, 7> names{
        diffuse, lightmap, environment, environment, environment, environment, environment};
    std::string key = "environment";
    key.push_back('\0');
    for (size_t i = 0; i < names.size(); ++i)
    {
        if (!load(names[i], item.assets[i], error)) return false;
        if (i < 2 && item.assets[i]->cube)
        {
            error = "environment texture slot " + std::to_string(i) +
                " requires 2D DDS: " + names[i];
            return false;
        }
        const auto it = std::find_if(assets_.begin(), assets_.end(),
            [asset = item.assets[i]](const auto& entry) { return &entry.second == asset; });
        R_ASSERT(it != assets_.end());
        item.paths[i] = it->first;
        key += it->first;
        key.push_back('\0');
    }
    if (item.assets[2]->cube != cube)
    {
        error = "environment texture '" + environment_name +
            (cube ? "' must be a cubemap DDS" : "' must be a 2D DDS");
        return false;
    }
    auto found = terrain_.find(key);
    if (found == terrain_.end())
    {
        std::array<VkImageView, 7> views{};
        for (size_t i = 0; i < views.size(); ++i) views[i] = item.assets[i]->texture.view;
        if (!pass.terrain_material(views, sampler_, item.set, error)) return false;
        item.pass = &pass;
        for (size_t i = 0; i < item.assets.size(); ++i)
            if (std::find(item.assets.begin(), item.assets.begin() + i, item.assets[i]) == item.assets.begin() + i)
                ++item.assets[i]->lightmap_refs;
        found = terrain_.emplace(std::move(key), std::move(item)).first;
    }
    R_ASSERT2(found->second.pass == &pass, "Vulkan environment material belongs to another pass");
    ++found->second.refs;
    result = found->second.set;
    error.clear();
    return true;
}

void GameTextureFactory::release_terrain_material(VkDescriptorSet set, DeferredPass& pass)
{
    if (!set) return;
    for (auto it = terrain_.begin(); it != terrain_.end(); ++it)
    {
        auto& item = it->second;
        if (item.set != set) continue;
        R_ASSERT2(item.pass == &pass && item.refs, "unknown Vulkan terrain lease");
        if (--item.refs) return;
        R_ASSERT2(retirement_idle(device_, wait_idle_), "Vulkan terrain retirement requires idle GPU frames");
        pass.release_gbuffer(set);
        for (size_t i = 0; i < item.assets.size(); ++i)
            if (std::find(item.assets.begin(), item.assets.begin() + i, item.assets[i]) == item.assets.begin() + i)
                --item.assets[i]->lightmap_refs;
        const auto paths = item.paths;
        terrain_.erase(it);
        for (const auto& path : paths) evict_if_unused(path);
        return;
    }
    R_ASSERT2(false, "unknown Vulkan terrain descriptor");
}

float GameTextureFactory::terrain_detail_scale(const std::string& diffuse) const
{
    // Same THM_CHUNK_DETAIL_EXT as STextureParams::Load. FS resolves archived
    // game data and loose mod overrides without requiring an extracted game.
    string_path path;
    const std::string name = trim_texture_name(diffuse);
    const bool found = FS.exist(path, "$level$", name.c_str(), ".thm") ||
        FS.exist(path, "$game_textures$", name.c_str(), ".thm");
    if (!found) return 1.f;
    IReader* reader = FS.r_open(path);
    if (!reader) return 1.f;
    float scale = 1.f;
    const size_t chunk_size = std::min(reader->find_chunk(0x0815),
        static_cast<size_t>(std::max<intptr_t>(0, reader->elapsed())));
    if (chunk_size)
    {
        const auto* bytes = static_cast<const char*>(reader->pointer());
        const auto* end = std::memchr(bytes, 0, chunk_size);
        if (end && static_cast<size_t>(static_cast<const char*>(end) - bytes) +
                1 + sizeof(float) <= chunk_size)
        {
            float value;
            std::memcpy(&value, static_cast<const char*>(end) + 1, sizeof(value));
            if (std::isfinite(value) && value > 0.f) scale = value;
        }
    }
    FS.r_close(reader);
    return scale;
}

bool GameTextureFactory::texture_detail_metadata(const std::string& diffuse,
    DetailMetadata& detail, std::string& error) const
{
    detail = {};
    const std::string name = trim_texture_name(diffuse);
    string_path path;
    if (!FS.exist(path, "$level$", name.c_str(), ".thm") &&
        !FS.exist(path, "$game_textures$", name.c_str(), ".thm"))
    { error.clear(); return true; } // Metadata is optional for implicit detail.
    IReader* reader = FS.r_open(path);
    if (!reader)
    { error = std::string(path) + " / texture THM cannot be opened"; return false; }
    const size_t parameter_size = reader->find_chunk(0x0812);
    const auto* params = parameter_size && reader->elapsed() >= 0 ?
        static_cast<const uint8_t*>(reader->pointer()) : nullptr;
    const size_t bounded_params = params ? std::min(parameter_size,
        static_cast<size_t>(reader->elapsed())) : 0;
    const size_t extension_size = reader->find_chunk(0x0815);
    const auto* extension = extension_size && reader->elapsed() >= 0 ?
        static_cast<const uint8_t*>(reader->pointer()) : nullptr;
    const size_t bounded_extension = extension ? std::min(extension_size,
        static_cast<size_t>(reader->elapsed())) : 0;
    const bool valid = decode_detail_metadata(params, bounded_params,
        extension, bounded_extension, detail, error);
    FS.r_close(reader);
    if (!valid) { error = std::string(path) + " / " + error; return false; }
    error.clear();
    return true;
}

bool GameTextureFactory::decode_detail_metadata(const uint8_t* params, size_t params_size,
    const uint8_t* extension, size_t extension_size,
    DetailMetadata& detail, std::string& error)
{
    detail = {};
    if (!params || params_size < 8)
    { error = "THM_CHUNK_TEXTUREPARAM is missing or truncated"; return false; }
    constexpr uint32_t diffuse_detail = 1u << 23, bump_detail = 1u << 26;
    const uint32_t flags = uint32_t(params[4]) | uint32_t(params[5]) << 8 |
        uint32_t(params[6]) << 16 | uint32_t(params[7]) << 24;
    detail.diffuse = (flags & diffuse_detail) != 0;
    detail.normal = (flags & bump_detail) != 0;
    if (extension && extension_size)
    {
        const auto* bytes = reinterpret_cast<const char*>(extension);
        const auto* end = static_cast<const char*>(std::memchr(bytes, 0, extension_size));
        if (!end || extension_size - (end - bytes) - 1 < sizeof(float))
        { error = "THM_CHUNK_DETAIL_EXT is truncated"; return false; }
        detail.texture.assign(bytes, end);
        std::memcpy(&detail.scale, end + 1, sizeof(float));
        if (!std::isfinite(detail.scale) || detail.scale <= 0.f)
        { error = "THM_CHUNK_DETAIL_EXT has invalid scale"; return false; }
    }
    if ((detail.diffuse || detail.normal) && detail.texture.empty())
    { error = "detail flags require THM_CHUNK_DETAIL_EXT / texture name"; return false; }
    error.clear();
    return true;
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
    if (it == assets_.end() || it->second.material_refs || it->second.clamp_material_refs ||
        it->second.lightmap_refs || it->second.ui_refs ||
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
        const bool clamp = asset.clamp_material_set == set;
        if (asset.material_set != set && !clamp) continue;
        size_t& refs = clamp ? asset.clamp_material_refs : asset.material_refs;
        R_ASSERT2(refs && asset.material_pass == &pass,
            "Vulkan material descriptor is not leased by this pass");
        if (--refs) return;
        R_ASSERT2(retirement_idle(device_, wait_idle_),
            "Vulkan device did not become idle before material release");
        pass.release_gbuffer(set);
        if (clamp) asset.clamp_material_set = VK_NULL_HANDLE;
        else asset.material_set = VK_NULL_HANDLE;
        if (!asset.material_set && !asset.clamp_material_set) asset.material_pass = nullptr;
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
        for (auto& [name, item] : terrain_)
            if (item.set && item.pass) item.pass->release_gbuffer(item.set);
        for (auto& [name, asset] : assets_)
        {
            if (asset.material_set && asset.material_pass)
                asset.material_pass->release_gbuffer(asset.material_set);
            if (asset.clamp_material_set && asset.material_pass)
                asset.material_pass->release_gbuffer(asset.clamp_material_set);
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
    terrain_.clear();
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
        if (it->second.material_refs || it->second.clamp_material_refs ||
            it->second.lightmap_refs || it->second.ui_refs || it->second.environment_refs)
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
        if (asset.clamp_material_set && asset.material_pass)
            asset.material_pass->update_material(asset.clamp_material_set, item.texture.view, ui_sampler_);
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
    for (auto& [name, item] : terrain_)
    {
        std::array<VkImageView, 7> views{};
        for (size_t i = 0; i < views.size(); ++i) views[i] = item.assets[i]->texture.view;
        item.pass->update_terrain_material(item.set, views, sampler_);
    }
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
