#include "xrEngine/stdafx.h"
#include "VulkanVideoTexture.h"
#include "ScenePass.h"

#include <vector>

namespace xray::render::vulkan
{
VulkanVideoTexture::~VulkanVideoTexture()
{
    if (descriptor_) textures_.release_ui(descriptor_, pass_);
}

bool VulkanVideoTexture::load(const char* path, u32 time, std::string& error)
{
    if (!surface_.Load(path)) { error = "could not decode OGM video"; return false; }
    surface_.ForceSoftwareRGB();
    extent_ = {surface_.Width(true), surface_.Height(true)};
    if (!extent_.width || !extent_.height || extent_.width > 8192 || extent_.height > 8192)
    { error = "OGM dimensions are invalid"; return false; }
    std::vector<uint8_t> blank(size_t(extent_.width) * extent_.height * 4);
    if (!textures_.ui_pixels(blank.data(), extent_.width, extent_.height,
            pass_, descriptor_, error)) return false;
    const bool stop_at_end = strstr(path, "intro\\") || strstr(path, "outro\\") ||
        strstr(path, "intro/") || strstr(path, "outro/");
    surface_.Play(!stop_at_end, time);
    return true;
}

void VulkanVideoTexture::play(bool looped, u32 time)
{
    sync_time_ = time;
    surface_.Play(looped, time);
}

void VulkanVideoTexture::update(u32 time)
{
    if (!surface_.Valid() || !surface_.Update(sync_time_ == 0xFFFFFFFF ? time : sync_time_))
        return;
    const size_t count = size_t(extent_.width) * extent_.height;
    std::vector<u32> argb(count);
    int decoded = 0;
    surface_.DecompressFrame(argb.data(), 0, decoded);
    if (decoded != static_cast<int>(count)) return;
    std::vector<uint8_t> rgba(count * 4);
    for (size_t i = 0; i < count; ++i)
    {
        rgba[i * 4] = uint8_t(argb[i] >> 16);
        rgba[i * 4 + 1] = uint8_t(argb[i] >> 8);
        rgba[i * 4 + 2] = uint8_t(argb[i]);
        rgba[i * 4 + 3] = uint8_t(argb[i] >> 24);
    }
    VkDescriptorSet replacement{};
    std::string error;
    if (textures_.ui_pixels(rgba.data(), extent_.width, extent_.height,
            pass_, replacement, error))
    {
        textures_.release_ui(descriptor_, pass_);
        descriptor_ = replacement;
    }
    else Msg("! [renderer-vulkan] OGM frame: %s", error.c_str());
}
}
