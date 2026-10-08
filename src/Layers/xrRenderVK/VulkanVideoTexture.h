#pragma once

#include "GameTextureFactory.h"
#include "xrEngine/xrTheora_Surface.h"

namespace xray::render::vulkan
{
class VulkanVideoTexture
{
public:
    VulkanVideoTexture(GameTextureFactory& textures, ScenePass& pass)
        : textures_(textures), pass_(pass) {}
    ~VulkanVideoTexture();
    bool load(const char* path, u32 time, std::string& error);
    void update(u32 time);
    void sync(u32 time) { sync_time_ = time; }
    void play(bool looped, u32 time);
    void stop() { surface_.Stop(); }
    bool playing() { return surface_.IsPlaying(); }
    VkDescriptorSet descriptor() const { return descriptor_; }
    VkExtent2D extent() const { return extent_; }

private:
    GameTextureFactory& textures_;
    ScenePass& pass_;
    CTheoraSurface surface_;
    VkDescriptorSet descriptor_{};
    VkExtent2D extent_{};
    u32 sync_time_{0xFFFFFFFF};
};
}
