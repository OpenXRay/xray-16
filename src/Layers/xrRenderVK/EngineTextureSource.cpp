#include "EngineTextureSource.h"

namespace xray::render::vulkan
{
bool upload_engine_texture(VkDevice device, VkQueue queue, VkCommandPool pool,
    const VkPhysicalDeviceMemoryProperties& memory_types, const TextureUploadDispatch& vk,
    const void* bytes, size_t size, bool bc_supported, UploadedTexture& result,
    std::vector<PendingTextureUpload>& pending, ImageStateTracker& states, std::string& error,
    VkExtent3D* decoded_extent)
{
    DdsTexture decoded;
    if (!decode_dds(bytes, size, bc_supported, decoded, error))
        return false;
    if (!upload_texture(device, queue, pool, memory_types, vk, decoded, result, pending, states, error))
        return false;
    if (decoded_extent) *decoded_extent = decoded.extent;
    return true;
}
}
