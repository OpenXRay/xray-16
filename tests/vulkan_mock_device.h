#pragma once
#include "src/Layers/xrRenderVK/BufferUpload.h"
#include "src/Layers/xrRenderVK/ScenePass.h"
#include "src/Layers/xrRenderVK/TextureUpload.h"
#include <cassert>
#include <type_traits>
#include <unordered_map>
using namespace xray::render::vulkan;
namespace vk_mock
{
template <class T> T handle(uintptr_t n)
{
    if constexpr (std::is_pointer_v<T>)
        return reinterpret_cast<T>(n);
    else
        return static_cast<T>(n);
}
uintptr_t next_handle = 100;
uint32_t draws{}, total_indices{}, buffers_destroyed{}, images_destroyed{}, submits{}, descriptor_frees{};
std::unordered_map<VkBuffer, VkDeviceSize> buffers;
std::unordered_map<VkDeviceMemory, std::vector<uint8_t>> allocations;
std::vector<VkDescriptorSet> bound_textures;
std::unordered_map<VkImage, VkDeviceSize> images;
std::unordered_map<VkDescriptorSet, unsigned> refs;

VkResult VKAPI_PTR create_buffer(VkDevice, const VkBufferCreateInfo *info, const VkAllocationCallbacks *, VkBuffer *out)
{
    *out = handle<VkBuffer>(next_handle++);
    buffers[*out] = info->size;
    return VK_SUCCESS;
}
void VKAPI_PTR destroy_buffer(VkDevice, VkBuffer b, const VkAllocationCallbacks *)
{
    assert(buffers.erase(b) == 1);
    ++buffers_destroyed;
}
void VKAPI_PTR requirements(VkDevice, VkBuffer b, VkMemoryRequirements *out)
{
    out->size = buffers.at(b);
    out->alignment = 16;
    out->memoryTypeBits = 1;
}
VkResult VKAPI_PTR allocate(VkDevice, const VkMemoryAllocateInfo *info, const VkAllocationCallbacks *, VkDeviceMemory *out)
{
    *out = handle<VkDeviceMemory>(next_handle++);
    allocations[*out].resize(info->allocationSize);
    return VK_SUCCESS;
}
void VKAPI_PTR free_memory(VkDevice, VkDeviceMemory m, const VkAllocationCallbacks *)
{
    assert(allocations.erase(m) == 1);
}
VkResult VKAPI_PTR bind_memory(VkDevice, VkBuffer, VkDeviceMemory, VkDeviceSize)
{
    return VK_SUCCESS;
}
VkResult VKAPI_PTR map(VkDevice, VkDeviceMemory m, VkDeviceSize, VkDeviceSize, VkMemoryMapFlags, void **out)
{
    *out = allocations.at(m).data();
    return VK_SUCCESS;
}
void VKAPI_PTR unmap(VkDevice, VkDeviceMemory)
{
}
VkResult VKAPI_PTR layout(VkDevice, const VkPipelineLayoutCreateInfo *, const VkAllocationCallbacks *, VkPipelineLayout *out)
{
    *out = handle<VkPipelineLayout>(next_handle++);
    return VK_SUCCESS;
}
void VKAPI_PTR destroy_layout(VkDevice, VkPipelineLayout, const VkAllocationCallbacks *)
{
}
VkResult VKAPI_PTR pipeline(VkDevice, VkPipelineCache, uint32_t, const VkGraphicsPipelineCreateInfo *info, const VkAllocationCallbacks *, VkPipeline *out)
{
    assert(info->stageCount == 2);
    *out = handle<VkPipeline>(next_handle++);
    return VK_SUCCESS;
}
void VKAPI_PTR destroy_pipeline(VkDevice, VkPipeline, const VkAllocationCallbacks *)
{
}
void VKAPI_PTR bind_pipeline(VkCommandBuffer, VkPipelineBindPoint, VkPipeline)
{
}
void VKAPI_PTR viewport(VkCommandBuffer, uint32_t, uint32_t, const VkViewport *v)
{
    assert(v->width > 0 && v->height > 0);
}
void VKAPI_PTR scissor(VkCommandBuffer, uint32_t, uint32_t, const VkRect2D *r)
{
    assert(r->extent.width > 0 && r->extent.height > 0);
}
void VKAPI_PTR bind_vertices(VkCommandBuffer, uint32_t, uint32_t, const VkBuffer *b, const VkDeviceSize *)
{
    assert(buffers.count(*b));
}
void VKAPI_PTR bind_indices(VkCommandBuffer, VkBuffer b, VkDeviceSize, VkIndexType type)
{
    assert(buffers.count(b) && type == VK_INDEX_TYPE_UINT32);
}
void VKAPI_PTR push(VkCommandBuffer, VkPipelineLayout, VkShaderStageFlags, uint32_t, uint32_t size, const void *data)
{
    assert(size && data);
}
void VKAPI_PTR draw(VkCommandBuffer, uint32_t count, uint32_t instances, uint32_t, int32_t, uint32_t)
{
    assert(instances == 1 && count % 3 == 0);
    ++draws;
    total_indices += count;
}
VkResult VKAPI_PTR descriptor_layout(VkDevice, const VkDescriptorSetLayoutCreateInfo *, const VkAllocationCallbacks *, VkDescriptorSetLayout *out)
{
    *out = handle<VkDescriptorSetLayout>(next_handle++);
    return VK_SUCCESS;
}
void VKAPI_PTR destroy_descriptor_layout(VkDevice, VkDescriptorSetLayout, const VkAllocationCallbacks *)
{
}
VkResult VKAPI_PTR pool(VkDevice, const VkDescriptorPoolCreateInfo *, const VkAllocationCallbacks *, VkDescriptorPool *out)
{
    *out = handle<VkDescriptorPool>(next_handle++);
    return VK_SUCCESS;
}
void VKAPI_PTR destroy_pool(VkDevice, VkDescriptorPool, const VkAllocationCallbacks *)
{
}
VkResult VKAPI_PTR descriptor(VkDevice, const VkDescriptorSetAllocateInfo *, VkDescriptorSet *out)
{
    *out = handle<VkDescriptorSet>(next_handle++);
    return VK_SUCCESS;
}
VkResult VKAPI_PTR free_descriptor(VkDevice, VkDescriptorPool, uint32_t, const VkDescriptorSet *)
{
    ++descriptor_frees;
    return VK_SUCCESS;
}
void VKAPI_PTR update(VkDevice, uint32_t, const VkWriteDescriptorSet *, uint32_t, const VkCopyDescriptorSet *)
{
}
void VKAPI_PTR bind_descriptor(VkCommandBuffer, VkPipelineBindPoint, VkPipelineLayout, uint32_t, uint32_t count, const VkDescriptorSet *set, uint32_t,
                               const uint32_t *)
{
    assert(count >= 1 && *set);
    bound_textures.push_back(*set);
}

VkResult VKAPI_PTR create_image(VkDevice, const VkImageCreateInfo *info, const VkAllocationCallbacks *, VkImage *out)
{
    *out = handle<VkImage>(next_handle++);
    images[*out] = info->extent.width * info->extent.height * 4;
    return VK_SUCCESS;
}
void VKAPI_PTR destroy_image(VkDevice, VkImage image, const VkAllocationCallbacks *)
{
    assert(images.erase(image) == 1);
    ++images_destroyed;
}
void VKAPI_PTR image_requirements(VkDevice, VkImage image, VkMemoryRequirements *out)
{
    out->size = images.at(image);
    out->alignment = 16;
    out->memoryTypeBits = 1;
}
VkResult VKAPI_PTR bind_image(VkDevice, VkImage, VkDeviceMemory, VkDeviceSize)
{
    return VK_SUCCESS;
}
VkResult VKAPI_PTR image_view(VkDevice, const VkImageViewCreateInfo *info, const VkAllocationCallbacks *, VkImageView *out)
{
    assert(images.count(info->image));
    *out = handle<VkImageView>(next_handle++);
    return VK_SUCCESS;
}
void VKAPI_PTR destroy_view(VkDevice, VkImageView, const VkAllocationCallbacks *)
{
}
VkResult VKAPI_PTR commands(VkDevice, const VkCommandBufferAllocateInfo *info, VkCommandBuffer *out)
{
    assert(info->commandBufferCount == 1);
    *out = handle<VkCommandBuffer>(next_handle++);
    return VK_SUCCESS;
}
void VKAPI_PTR free_commands(VkDevice, VkCommandPool, uint32_t, const VkCommandBuffer *)
{
}
VkResult VKAPI_PTR begin(VkCommandBuffer, const VkCommandBufferBeginInfo *)
{
    return VK_SUCCESS;
}
VkResult VKAPI_PTR end(VkCommandBuffer)
{
    return VK_SUCCESS;
}
void VKAPI_PTR copy_buffer(VkCommandBuffer, VkBuffer a, VkBuffer b, uint32_t n, const VkBufferCopy *copies)
{
    assert(buffers.count(a) && buffers.count(b) && n == 1 && copies->size <= buffers.at(b));
}
void VKAPI_PTR barrier(VkCommandBuffer, VkPipelineStageFlags, VkPipelineStageFlags, VkDependencyFlags, uint32_t, const VkMemoryBarrier *, uint32_t,
                       const VkBufferMemoryBarrier *, uint32_t, const VkImageMemoryBarrier *)
{
}
void VKAPI_PTR copy_image(VkCommandBuffer, VkBuffer b, VkImage image, VkImageLayout, uint32_t n, const VkBufferImageCopy *)
{
    assert(buffers.count(b) && images.count(image) && n > 0);
}
VkResult VKAPI_PTR submit(VkQueue, uint32_t n, const VkSubmitInfo *info, VkFence fence)
{
    assert(n == 1 && info->commandBufferCount == 1 && fence);
    ++submits;
    return VK_SUCCESS;
}
VkResult VKAPI_PTR fence(VkDevice, const VkFenceCreateInfo *, const VkAllocationCallbacks *, VkFence *out)
{
    *out = handle<VkFence>(next_handle++);
    return VK_SUCCESS;
}
void VKAPI_PTR destroy_fence(VkDevice, VkFence, const VkAllocationCallbacks *)
{
}
VkResult VKAPI_PTR fence_status(VkDevice, VkFence)
{
    return VK_SUCCESS;
}
VkResult VKAPI_PTR wait(VkDevice, uint32_t, const VkFence *, VkBool32, uint64_t)
{
    return VK_SUCCESS;
}
VkResult VKAPI_PTR idle(VkDevice)
{
    return VK_SUCCESS;
}
VkResult VKAPI_PTR sampler(VkDevice, const VkSamplerCreateInfo *, const VkAllocationCallbacks *, VkSampler *out)
{
    *out = handle<VkSampler>(next_handle++);
    return VK_SUCCESS;
}
void VKAPI_PTR destroy_sampler(VkDevice, VkSampler, const VkAllocationCallbacks *)
{
}
void VKAPI_PTR draw_plain(VkCommandBuffer, uint32_t, uint32_t, uint32_t, uint32_t)
{
}
ScenePassDispatch scene_dispatch()
{
    ScenePassDispatch result{
        layout,         destroy_layout, pipeline,          destroy_pipeline,          bind_pipeline, viewport,     scissor,    bind_vertices,   bind_indices,
        push,           draw,           descriptor_layout, destroy_descriptor_layout, pool,          destroy_pool, descriptor, free_descriptor, update,
        bind_descriptor};
    result.cmd_draw = draw_plain;
    return result;
}
BufferResourceDispatch buffer_dispatch()
{
    return {create_buffer, destroy_buffer, requirements, allocate, free_memory, bind_memory, map, unmap};
}
BufferUploadDispatch upload_dispatch()
{
    return {buffer_dispatch(), commands, free_commands, begin, end, copy_buffer, barrier, submit, fence, destroy_fence, fence_status, wait};
}
TextureUploadDispatch texture_dispatch()
{
    return {create_buffer, destroy_buffer, requirements,  create_image, destroy_image, image_requirements, allocate, free_memory, bind_memory, bind_image,
            map,           unmap,          image_view,    destroy_view, commands,      free_commands,      begin,    end,         barrier,     copy_image,
            submit,        fence,          destroy_fence, fence_status, wait};
}
} // namespace vk_mock
