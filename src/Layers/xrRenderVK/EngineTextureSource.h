#pragma once

#include "TextureUpload.h"

namespace xray::render::vulkan
{
// The caller owns filesystem lookup and the reader. These bytes may come from
// loose game files, archives, or mod overrides; no host filesystem path is needed.
bool upload_engine_texture(VkDevice device, VkQueue queue, VkCommandPool pool,
    const VkPhysicalDeviceMemoryProperties& memory_types, const TextureUploadDispatch& vk,
    const void* bytes, size_t size, bool bc_supported, UploadedTexture& result,
    std::vector<PendingTextureUpload>& pending, ImageStateTracker& states, std::string& error,
    VkExtent3D* decoded_extent = nullptr);
}
