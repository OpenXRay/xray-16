#include "src/xrEngine/stdafx.h"
#include "src/Layers/xrRenderVK/VulkanFramePhaseState.h"
#include "src/Layers/xrRenderVK/VulkanUIRender.h"
#include <cassert>
#include <type_traits>
#include <unordered_map>

using namespace xray::render::vulkan;

namespace
{
template <class T> T handle(uintptr_t n)
{
    if constexpr (std::is_pointer_v<T>)
        return reinterpret_cast<T>(n);
    else
        return static_cast<T>(n);
}
uintptr_t next_handle = 100;
uint32_t draws{}, total_indices{}, buffers_destroyed{};
std::unordered_map<VkBuffer, VkDeviceSize> buffers;
std::unordered_map<VkDeviceMemory, std::vector<uint8_t>> allocations;
std::vector<VkDescriptorSet> bound_textures;
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
    assert(v->width == 960 && v->height == 540);
}
void VKAPI_PTR scissor(VkCommandBuffer, uint32_t, uint32_t, const VkRect2D *r)
{
    assert(r->extent.width <= 960 && r->extent.height <= 540);
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
    assert(size == 12 && data);
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
    return VK_SUCCESS;
}
void VKAPI_PTR update(VkDevice, uint32_t, const VkWriteDescriptorSet *, uint32_t, const VkCopyDescriptorSet *)
{
}
void VKAPI_PTR bind_descriptor(VkCommandBuffer, VkPipelineBindPoint, VkPipelineLayout, uint32_t, uint32_t count, const VkDescriptorSet *set, uint32_t,
                               const uint32_t *)
{
    assert(count == 1 && refs.at(*set) > 0);
    bound_textures.push_back(*set);
}

void quad(VulkanUIRender &ui, VkDescriptorSet texture, float x)
{
    ui.SetTextureDescriptor(texture);
    ui.StartPrimitive(4, IUIRender::ptTriStrip, IUIRender::pttTL);
    ui.PushPoint(x, 10, 0, 0xffffffff, 0, 0);
    ui.PushPoint(x, 30, 0, 0xffffffff, 0, 1);
    ui.PushPoint(x + 20, 10, 0, 0xffffffff, 1, 0);
    ui.PushPoint(x + 20, 30, 0, 0xffffffff, 1, 1);
    ui.FlushPrimitive();
}
} // namespace

int main()
{
    ScenePassDispatch scene{
        layout,         destroy_layout, pipeline,          destroy_pipeline,          bind_pipeline, viewport,     scissor,    bind_vertices,   bind_indices,
        push,           draw,           descriptor_layout, destroy_descriptor_layout, pool,          destroy_pool, descriptor, free_descriptor, update,
        bind_descriptor};
    const auto device = handle<VkDevice>(1);
    const auto render_pass = handle<VkRenderPass>(2);
    ScenePass pass;
    std::string error;
    assert(pass.initialize(device, render_pass, handle<VkShaderModule>(3), handle<VkShaderModule>(4), handle<VkShaderModule>(5), handle<VkShaderModule>(6),
                           scene, error, true));
    BufferResourceDispatch buffer{create_buffer, destroy_buffer, requirements, allocate, free_memory, bind_memory, map, unmap};
    VkPhysicalDeviceMemoryProperties memory{};
    memory.memoryTypeCount = 1;
    memory.memoryTypes[0].propertyFlags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
    VulkanUIRender ui;
    ui.configure(device, memory, buffer, pass,
                 {[](VkDescriptorSet s) {
                      auto it = refs.find(s);
                      if (it == refs.end() || !it->second)
                          return false;
                      ++it->second;
                      return true;
                  },
                  [](VkDescriptorSet s) {
                      assert(refs.at(s));
                      --refs.at(s);
                  }});
    ui.CreateUIGeom();
    const auto font = handle<VkDescriptorSet>(10), progress = handle<VkDescriptorSet>(11);
    refs[font] = refs[progress] = 1;
    FrameRecordingContext frame{handle<VkCommandBuffer>(7), render_pass, handle<VkFramebuffer>(8), {960, 540}, 0, 0};
    VulkanFramePhaseState phase;
    // Loading has font/progress UI before any level or world calculation.
    assert(phase.begin());
    quad(ui, font, 10);
    quad(ui, progress, 40);
    assert(ui.draw_calls() == 2 && ui.triangles() == 4 && phase.can_end());
    assert(ui.record(frame, error) && phase.end());
    assert(draws == 2 && total_indices == 12);
    ui.reset_frame();
    assert(refs[font] == 1 && refs[progress] == 1 && ui.draw_calls() == 0);
    // Two movie frames and a button keep separate descriptors even when the
    // movie producer replaces its frame and the menu is destroyed mid-frame.
    const auto movie_a = handle<VkDescriptorSet>(12), movie_b = handle<VkDescriptorSet>(13);
    refs[movie_a] = refs[movie_b] = 1;
    assert(phase.begin() && phase.render_menu());
    quad(ui, movie_a, 10);
    --refs[movie_a]; // video replaced its frame
    quad(ui, movie_b, 40);
    quad(ui, font, 70);
    --refs[movie_b];
    --refs[font]; // menu owners destroyed after enqueue
    assert(refs[movie_a] == 1 && refs[movie_b] == 1 && refs[font] == 1);
    frame.frame_index = 1;
    assert(ui.record(frame, error) && phase.end());
    assert(bound_textures[2] == movie_a && bound_textures[3] == movie_b && bound_textures[4] == font);
    ui.setup_states();
    assert(!refs[movie_a] && !refs[movie_b] && !refs[font]);
    assert(ui.draw_calls() == 0 && ui.triangles() == 0);
    // Cancel/reopen and graphics reset start with clean batches/scissor/state.
    assert(phase.begin() && phase.render_menu());
    Irect clip;
    clip.set(0, 0, 60, 60);
    ui.SetScissor(&clip);
    ui.SetAlphaRef(127);
    quad(ui, progress, 10);
    frame.frame_index = 0;
    assert(ui.record(frame, error) && phase.end());
    ui.DestroyUIGeom();
    assert(refs[progress] == 1 && buffers.empty() && allocations.empty() && buffers_destroyed == 4);
    ui.CreateUIGeom();
    assert(ui.record(frame, error) && ui.draw_calls() == 0);
    ui.DestroyUIGeom();
    pass.destroy();
}
