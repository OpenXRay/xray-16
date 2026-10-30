#include "vulkan_mock_device.h"
#include <iostream>
namespace {
unsigned waits{},waited_fences{};
VkResult VKAPI_PTR pending_fence(VkDevice,VkFence) { return VK_NOT_READY; }
VkResult VKAPI_PTR wait_batch(VkDevice,uint32_t count,const VkFence* fences,VkBool32 all,uint64_t) {
    assert(count && fences && all);++waits;waited_fences+=count;return VK_SUCCESS;
}
}
int main() {
    const auto device=vk_mock::handle<VkDevice>(1);
    const auto queue=vk_mock::handle<VkQueue>(2);
    const auto pool=vk_mock::handle<VkCommandPool>(3);
    VkPhysicalDeviceMemoryProperties memory{};memory.memoryTypeCount=1;
    memory.memoryTypes[0].propertyFlags=VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT|VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT|VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
    DdsTexture data;data.format=VK_FORMAT_R8G8B8A8_UNORM;data.extent={2,2,1};data.mip_levels=1;
    data.pixels.resize(16,255);VkBufferImageCopy copy{};
    copy.imageSubresource={VK_IMAGE_ASPECT_COLOR_BIT,0,0,1};copy.imageExtent={2,2,1};data.copies.push_back(copy);
    for(bool deferred:{false,true}) {
        waits=waited_fences=0;auto vk=vk_mock::texture_dispatch();
        vk.fence_after_copy=true;vk.defer_fence_wait=deferred;vk.get_fence_status=pending_fence;vk.wait_for_fences=wait_batch;
        std::vector<PendingTextureUpload> pending;ImageStateTracker states;std::string error;
        std::array<UploadedTexture,4> textures;
        for(auto& texture:textures)
            assert(upload_texture(device,queue,pool,memory,vk,data,texture,pending,states,error));
        assert(waits==(deferred?0u:4u) && pending.size()==(deferred?4u:0u));
        assert(wait_for_uploads(device,pool,vk,pending));
        assert(waits==(deferred?1u:4u) && waited_fences==4 && pending.empty());
        for(auto& texture:textures) destroy_texture(device,vk,texture);
    }
    assert(vk_mock::allocations.empty() && vk_mock::buffers.empty() && vk_mock::images.empty());
    std::cout<<"Adreno workaround retained: four uploads wait once before sampling, rather than after each copy\n";
}
